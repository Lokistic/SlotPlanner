#pragma once
#include <string>
#include <vector>
#include <random>
#include <chrono>
#include <algorithm>

struct ExtraBet {
    std::string name;
    float rtp = 0.95f;      // 0..1
    float cost_mult = 0.00f;// extra cost in multiples of base bet
    bool enabled = false;
};

struct Game {
    std::string name;
    float rtp = 0.96f;      // 0..1
    float hit_rate = 0.25f; // 0..1
    float volatility = 0.6f;// 0..1
    float max_win_x = 5000;
    std::vector<ExtraBet> extras;
};

enum class RiskProfile { Conservative, Balanced, Aggressive };

struct SessionInput {
    float start_bankroll = 100.0f;
    int target_minutes = 0;
    bool include_time = false;
    int spins_per_min = 12;
    int trials = 2000;
    int max_spins_cap = 2000;
    bool lock_bet_size = false;
    float user_bet_size = 1.0f;
    RiskProfile risk = RiskProfile::Balanced;
};

struct SimResult {
    float recommended_bet = 0.0f;
    int planned_spins = 0;
    float prob_ruin = 0.0f;
    float prob_hit_target = 0.0f;
    float expected_end = 0.0f;
    float stop_loss = 0.0f;
    float take_profit = 0.0f;
    float expected_loss_per_spin = 0.0f;
};

inline std::mt19937& RNG() {
    static thread_local std::mt19937 rng{ (unsigned)std::chrono::high_resolution_clock::now().time_since_epoch().count() };
    return rng;
}