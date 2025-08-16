#pragma once
#include "Models.h"
#include <cmath>
#include <random>
#include <numeric>

inline void ComputeEffectiveGame(const Game& g, float& rtp_eff, float& cost_mult_eff) {
	rtp_eff = g.rtp;
	cost_mult_eff = 1.0f;
	for (const auto& e : g.extras) if (e.enabled) {
		float extra_cost = e.cost_mult;
		float old_cost = cost_mult_eff;
		cost_mult_eff += extra_cost;
		rtp_eff = (rtp_eff * old_cost + extra_cost * e.rtp) / cost_mult_eff;
	}
}

inline float DrawPayoutMult(float mean_on_hit, float volatility, float max_x) {
	float sigma = 0.5f + 1.5f * std::clamp(volatility, 0.0f, 1.0f);
	float mu = std::log(std::max(1e-4f, mean_on_hit)) - 0.5f * sigma * sigma;
	std::lognormal_distribution<float> dist(mu, sigma);
	float x = dist(RNG());
	return x > max_x ? max_x : x;
}

inline float SuggestTakeProfit(float start, RiskProfile risk) {
	float m;
	if (start < 25) m = (risk == RiskProfile::Conservative ? 3.f : risk == RiskProfile::Balanced ? 4.f : 6.f);
	else if (start < 100) m = (risk == RiskProfile::Conservative ? 2.f : risk == RiskProfile::Balanced ? 2.5f : 3.5f);
	else if (start < 500) m = (risk == RiskProfile::Conservative ? 1.7f : risk == RiskProfile::Balanced ? 2.0f : 2.7f);
	else if (start < 1500) m = (risk == RiskProfile::Conservative ? 1.5f : risk == RiskProfile::Balanced ? 1.8f : 2.2f);
	else m = (risk == RiskProfile::Conservative ? 1.4f : risk == RiskProfile::Balanced ? 1.6f : 2.0f);
	return start * m;
}

inline float SuggestStopLoss(float start, RiskProfile risk) {
	float lp = (risk == RiskProfile::Conservative ? 0.30f : risk == RiskProfile::Balanced ? 0.45f : 0.65f);
	return start * (1.0f - lp);
}

inline float SuggestBetSize(float bankroll, float rtp_eff, float /*hit_rate*/, RiskProfile risk, int spins) {
	float exp_loss_1 = std::max(0.0f, 1.0f - rtp_eff);
	if (exp_loss_1 < 1e-4f) exp_loss_1 = 1e-4f;
	float safety = (risk == RiskProfile::Conservative ? 12.f : risk == RiskProfile::Balanced ? 8.f : 5.f);
	float bet = bankroll / (safety * std::max(1, spins) * exp_loss_1);
	return std::clamp(bet, 0.01f, bankroll * 0.10f);
}

inline SimResult SimulateSession(const Game& g, const SessionInput& in) {
	SimResult out{};
	float rtp_eff, cost_mult; ComputeEffectiveGame(g, rtp_eff, cost_mult);
	int spins = in.include_time && in.target_minutes > 0 ? in.target_minutes * std::max(1, in.spins_per_min) : in.max_spins_cap;
	out.planned_spins = spins;
	out.expected_loss_per_spin = cost_mult * (1.0f - rtp_eff);
	out.recommended_bet = in.lock_bet_size ? in.user_bet_size : SuggestBetSize(in.start_bankroll, rtp_eff, g.hit_rate, in.risk, spins);
	out.stop_loss = SuggestStopLoss(in.start_bankroll, in.risk);
	out.take_profit = SuggestTakeProfit(in.start_bankroll, in.risk);

	int trials = std::max(100, in.trials);
	std::bernoulli_distribution hit(g.hit_rate);

	int hit_tp = 0, ruin = 0; double end_sum = 0.0;
	for (int t = 0; t < trials; ++t) {
		double bank = in.start_bankroll;
		for (int s = 0; s < spins; ++s) {
			double bet_total = out.recommended_bet * cost_mult;
			if (bank < bet_total) break;
			bank -= bet_total;
			if (hit(RNG())) {
				float mean_on_hit = (rtp_eff * cost_mult) / std::max(0.001f, g.hit_rate);
				float mult = DrawPayoutMult(mean_on_hit, g.volatility, g.max_win_x);
				bank += out.recommended_bet * mult; // payout on base bet
			}
			if (bank >= out.take_profit) { ++hit_tp; break; }
			if (bank <= out.stop_loss) { ++ruin; break; }
		}
		end_sum += bank;
	}
	out.prob_hit_target = float(hit_tp) / trials;
	out.prob_ruin = float(ruin) / trials;
	out.expected_end = float(end_sum / trials);
	return out;
}

struct PathBands {
	int steps = 0;
	std::vector<float> p10, p25, p50, p75, p90;
};


// helper
inline float Percentile(std::vector<float>& v, float p) {
	if (v.empty()) return 0.f;
	std::sort(v.begin(), v.end());
	float idx = (p / 100.f) * (v.size() - 1);
	size_t i = (size_t)idx;
	float frac = idx - i;
	if (i + 1 < v.size()) return v[i] * (1.f - frac) + v[i + 1] * frac;
	return v.back();
}

inline float DrawPayoutMultMixture(float mean_on_hit, float volatility, float max_x) {
	float v = std::clamp(volatility, 0.f, 1.f);
	float w_big = 0.05f + 0.15f * v;         // 5% .. 20% big-hit chance
	float r = 6.0f + 24.0f * v;         // big 6x..30x the small mean

	float denom = (1.0f - w_big) + w_big * r;
	float m_small = mean_on_hit / std::max(denom, 1e-6f);
	float m_big = r * m_small;

	auto draw = [&](float mean) {
		float sigma = 0.45f + 1.7f * v;
		float mu = std::log(std::max(1e-4f, mean)) - 0.5f * sigma * sigma;
		std::lognormal_distribution<float> dist(mu, sigma);
		float x = dist(RNG());
		return std::min(x, max_x);
		};

	std::bernoulli_distribution big(w_big);
	return draw(big(RNG()) ? m_big : m_small);
}

inline PathBands SimulatePathBands(const Game& g, const SessionInput& in) {
	float rtp_eff, cost_mult; ComputeEffectiveGame(g, rtp_eff, cost_mult);
	int spins = in.include_time && in.target_minutes > 0 ? in.target_minutes * std::max(1, in.spins_per_min) : in.max_spins_cap;

	if (!in.include_time)
		spins = std::min(spins, std::max(50, int((in.start_bankroll / std::max(0.001f, cost_mult * (1.f - rtp_eff))) * 1.2f)));

	float bet = in.lock_bet_size ? in.user_bet_size : SuggestBetSize(in.start_bankroll, rtp_eff, g.hit_rate, in.risk, spins);
	float tp = SuggestTakeProfit(in.start_bankroll, in.risk);
	float sl = SuggestStopLoss(in.start_bankroll, in.risk);

	int trials = std::max(200, in.trials);
	std::bernoulli_distribution hit(g.hit_rate);

	std::vector<std::vector<float>> snapshots(spins + 1);
	for (auto& s : snapshots) s.reserve(trials);

	for (int t = 0; t < trials; ++t) {
		double bank = in.start_bankroll;
		double peak = bank;
		// ratcheting SL: track 25% of gains
		auto soft_stop = [&](double b) {
			double gain = std::max(0.0, b - in.start_bankroll);
			return std::max<double>(sl, in.start_bankroll + gain * 0.25);
			};

		snapshots[0].push_back(float(bank));
		for (int s = 0; s < spins; ++s) {
			double spin_cost = bet * cost_mult;
			if (bank < spin_cost) { // record flat until end
				for (int k = s + 1; k <= spins; ++k) snapshots[k].push_back(float(bank));
				break;
			}
			bank -= spin_cost;

			if (hit(RNG())) {
				float mean_on_hit = g.rtp / std::max(0.001f, g.hit_rate);
				float mult = DrawPayoutMultMixture(mean_on_hit, g.volatility, g.max_win_x);
				bank += bet * mult; // payout on base bet only
			}

			double peak = bank;
			const double trail_pct = 0.25;      // 25% of gains

			peak = std::max(peak, bank);
			double ts = sl + (peak - in.start_bankroll) * trail_pct;

			if (bank >= tp) {
				for (int k = s + 1; k <= spins; ++k) snapshots[k].push_back(float(tp));
				break;
			}
			if (bank <= ts) {
				for (int k = s + 1; k <= spins; ++k) snapshots[k].push_back(std::max((float)bank, sl));
				break;
			}
			snapshots[s + 1].push_back(float(bank));
		}
	}

	PathBands bands; bands.steps = spins + 1;
	bands.p10.resize(bands.steps);
	bands.p25.resize(bands.steps);
	bands.p50.resize(bands.steps);
	bands.p75.resize(bands.steps);
	bands.p90.resize(bands.steps);

	for (int i = 0; i <= spins; ++i) {
		auto v = std::move(snapshots[i]); // copy -> move to sort
		bands.p10[i] = Percentile(v, 10);
		bands.p25[i] = Percentile(v, 25);
		bands.p50[i] = Percentile(v, 50);
		bands.p75[i] = Percentile(v, 75);
		bands.p90[i] = Percentile(v, 90);
	}
	return bands;
}