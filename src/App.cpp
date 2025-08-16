#include "App.h"

#ifdef USE_IMPLOT
#include "implot.h"
#endif

static std::vector<Game> LoadDemoGames() {
	Game g1{ "Mental II", 0.9606f, 0.3141f, 0.95f, 99999.0f, {
		{"Xbet", 0.9609f, 0.40f, false},
		{"Bloodletting Spins (100x)", 0.9611f, 100.0f, false}
	} };
	Game g2{ "Reactoonz", 0.9651f, 0.42f, 0.55f, 4750.0f, {
	} };
	Game g3{ "Blood & Shadow 2", 0.9609f, 0.2714f, 0.85f, 16161.0f, {
		{"Xbet", 0.9605f, 1.50f, false},
		{"Bonus Buy (100x)", 0.9603f, 100.0f, false}
	} };
	return { g1,g2,g3 };
}

SlotPlannerApp::SlotPlannerApp() {
	games_ = LoadDemoGames();
}

void SlotPlannerApp::Draw() {

	ImGuiViewport* vp = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(vp->Pos);
	ImGui::SetNextWindowSize(vp->Size);

	ImGui::GetIO().IniFilename = nullptr;

	ImGui::Begin("##Root", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoSavedSettings);
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Exit")) {
				exit(0); //kek
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("View")) {
			ImGui::MenuItem("Advanced Panel", nullptr, &show_advanced_);
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	float w = ImGui::GetContentRegionAvail().x;
	float col_w = std::max(300.0f, w * 0.38f);

	ImGui::BeginGroup();
	ImGui::BeginChild("LeftPane", { col_w, 0 }, false);
	DrawLeftPane();
	ImGui::EndChild();
	ImGui::EndGroup();

	ImGui::SameLine();

	ImGui::BeginGroup();
	ImGui::BeginChild("RightPane", { 0, 0 }, false);
	DrawRightPane();
	ImGui::EndChild();
	ImGui::EndGroup();

	ImGui::End();
}

void SlotPlannerApp::DrawPlanSummary(const Game& g, const SessionInput& in, const SimResult& r) {
	BeginCard("Session Plan Summary");
	ImGui::Text("Game: %s", g.name.c_str());
	ImGui::Separator();
	ImGui::Text("Start bankroll: %.2f", in.start_bankroll);
	if (in.include_time && in.target_minutes > 0)
		ImGui::Text("Planned time: %d min (~%d spins)", in.target_minutes, r.planned_spins);
	else
		ImGui::Text("Planned spins: %d (time not specified)", r.planned_spins);
	ImGui::Text("Recommended bet: %.2f per spin", r.recommended_bet);
	ImGui::Text("Stop-loss: %.2f | Take-profit: %.2f", r.stop_loss, r.take_profit);
	ImGui::Text("Risk of busting before TP: %.1f%%", r.prob_ruin * 100.0f);
	ImGui::Text("Chance to hit target before SL: %.1f%%", r.prob_hit_target * 100.0f);
	ImGui::Text("Expected session end: %.2f", r.expected_end);

	if (!g.extras.empty()) {
		ImGui::Separator(); ImGui::TextUnformatted("Extras included:");
		bool any = false;
		for (auto& e : g.extras) if (e.enabled) { any = true; ImGui::BulletText("%s (RTP %.1f%%, cost +%.0f%%)", e.name.c_str(), e.rtp * 100.f, e.cost_mult * 100.f); }
		if (!any) ImGui::TextDisabled("(none)");
	}

	ImGui::Spacing();
	ImGui::TextDisabled("Heuristic notes:");
	ImGui::BulletText("Bet sizing scales with expected loss & planned spins.");
	ImGui::BulletText("Profit targets scale with bankroll (small stacks aim higher multiples).");
	ImGui::BulletText("No guarantees. Negative EV.");
	ImGui::Spacing();

#ifdef USE_IMPLOT

	// Manual refresh
	//if (ImGui::Button("Recompute bands")) { bands_dirty_ = true; }

	// Re-sim only if needed
	if (!bands_valid_ || bands_dirty_) {
		bands_ = SimulatePathBands(g, input_);
		bands_valid_ = true;
		bands_dirty_ = false;

		auto find_minmax = [](const std::vector<float>& v, float& mn, float& mx) {
			for (float f : v) { mn = std::min(mn, f); mx = std::max(mx, f); }
			};
		bands_ymin_ = FLT_MAX; bands_ymax_ = -FLT_MAX;
		find_minmax(bands_.p10, bands_ymin_, bands_ymax_);
		find_minmax(bands_.p90, bands_ymin_, bands_ymax_);

		float pad = std::max(1.0f, (bands_ymax_ - bands_ymin_) * 0.05f);
		bands_ymin_ -= pad; bands_ymax_ += pad;
	}

	static std::vector<float> x;
	x.resize(bands_.steps);
	for (int i = 0; i < bands_.steps; ++i) x[i] = (float)i;

	ImPlot::SetNextAxesLimits(0, (double)(bands_.steps - 1),
		(double)bands_ymin_, (double)bands_ymax_,
		ImGuiCond_Always);

	const ImPlotFlags plot_flags =
		ImPlotFlags_NoLegend | ImPlotFlags_NoBoxSelect |
		ImPlotFlags_NoMenus | ImPlotFlags_NoMouseText;

	const ImPlotAxisFlags axis_flags =
		ImPlotAxisFlags_Lock | ImPlotAxisFlags_NoGridLines |
		ImPlotAxisFlags_NoHighlight;

	if (ImPlot::BeginPlot("Session Bands", ImVec2(-1, 260), plot_flags)) {
		ImPlot::SetupAxes("Spin", "Bankroll", axis_flags, axis_flags);
		// ImPlot::SetupAxes("","", axis_flags | ImPlotAxisFlags_NoTickLabels,
		//                          axis_flags | ImPlotAxisFlags_NoTickLabels);

		ImPlot::PlotShaded("p10–p90", x.data(), bands_.p10.data(), bands_.p90.data(), bands_.steps);
		ImPlot::PlotLine("p50", x.data(), bands_.p50.data(), bands_.steps);
		ImPlot::EndPlot();
	}
#else
	ImGui::TextDisabled("ImPlot not compiled. Define USE_IMPLOT to enable charts.");
#endif

	EndCard();
}

void SlotPlannerApp::DrawLeftPane() {
	BeginCard("Setup");
	auto& g = games_[game_idx_];

	if (ImGui::BeginCombo("Game", g.name.c_str())) {
		for (int i = 0; i < (int)games_.size(); ++i) { bool sel = (i == game_idx_); if (ImGui::Selectable(games_[i].name.c_str(), sel)) game_idx_ = i; if (sel) ImGui::SetItemDefaultFocus(); }
		ImGui::EndCombo();
	}

	ImGui::TextDisabled("Base RTP: %.2f%% | Hit: %.1f%% | Vol: %.2f", g.rtp * 100.f, g.hit_rate * 100.f, g.volatility);
	ImGui::Separator();

	bool changed = false;

	ImGui::InputFloat("Starting balance", &input_.start_bankroll, 1.0f, 10.0f, "%.2f");
	if (input_.start_bankroll < 1.0f) input_.start_bankroll = 1.0f;

	ImGui::Checkbox("Specify play time (min)", &input_.include_time);
	if (input_.include_time) { ImGui::SliderInt("Minutes", &input_.target_minutes, 5, 240); ImGui::SliderInt("Spins/min", &input_.spins_per_min, 6, 30); }

	ImGui::TextUnformatted("Risk profile");
	int r = (int)input_.risk;
	ImGui::RadioButton("Conservative", &r, (int)RiskProfile::Conservative); ImGui::SameLine();
	ImGui::RadioButton("Balanced", &r, (int)RiskProfile::Balanced); ImGui::SameLine();
	ImGui::RadioButton("Aggressive", &r, (int)RiskProfile::Aggressive); input_.risk = (RiskProfile)r;

	if (changed) bands_dirty_ = true;

	ImGui::Separator();

	if (!g.extras.empty()) {
		ImGui::TextUnformatted("Extras (side bets)");
		for (auto& e : g.extras) { ImGui::PushID(&e); ImGui::Checkbox(e.name.c_str(), &e.enabled); ImGui::SameLine(); ImGui::TextDisabled("RTP %.1f%%, cost +%.0f%%", e.rtp * 100.f, e.cost_mult * 100.f); ImGui::PopID(); }
	}

	ImGui::Separator();

	ImGui::Checkbox("Lock my bet size", &input_.lock_bet_size);
	if (input_.lock_bet_size) { ImGui::InputFloat("My bet", &input_.user_bet_size, 0.1f, 1.0f, "%.2f"); if (input_.user_bet_size < 0.01f) input_.user_bet_size = 0.01f; }

	ImGui::Separator();

	if (ImGui::Button("Run Simulation", { -1, 0 })) { result_ = SimulateSession(g, input_); has_result_ = true; bands_dirty_ = true; }

	EndCard();
}

void SlotPlannerApp::DrawRightPane() {
	auto& g = games_[game_idx_];
	if (has_result_) DrawPlanSummary(g, input_, result_);
	else { BeginCard("Session Plan Summary"); ImGui::TextWrapped("Set bankroll, pick a game, and Run Simulation for bet sizing, stop-loss, and profit target."); EndCard(); }

	ImGui::Spacing();

	BeginCard("Advanced");
	ImGui::TextDisabled("Tuning & what-if analysis");
	ImGui::SliderInt("Trials", &input_.trials, 500, 20000);
	ImGui::SliderInt("Max spins cap (if no time)", &input_.max_spins_cap, 100, 5000);

	if (ImGui::CollapsingHeader("Edit current game stats")) {
		ImGui::InputFloat("Base RTP", &g.rtp, 0.001f, 0.01f, "%.3f");
		ImGui::SliderFloat("Hit rate", &g.hit_rate, 0.02f, 0.60f, "%.2f");
		ImGui::SliderFloat("Volatility", &g.volatility, 0.10f, 0.95f, "%.2f");
		ImGui::InputFloat("Max win (x)", &g.max_win_x, 10.0f, 100.0f, "%.0f");
	}

	if (ImGui::CollapsingHeader("Add custom extra bet")) {
		ImGui::InputText("Name", custom_extra_name_, IM_ARRAYSIZE(custom_extra_name_));
		ImGui::SliderFloat("RTP", &custom_extra_rtp_, 0.80f, 0.99f, "%.2f");
		ImGui::SliderFloat("Cost (+x base)", &custom_extra_cost_, 0.05f, 100.0f, "%.2f");
		if (ImGui::Button("Add to game")) {
			if (custom_extra_name_[0]) { games_[game_idx_].extras.push_back({ custom_extra_name_, custom_extra_rtp_, custom_extra_cost_, true }); custom_extra_name_[0] = '\0'; }
		}
	}

	if (has_result_) {
		ImGui::Separator(); ImGui::TextDisabled("Interpretation");
		ImGui::BulletText("Expected loss per spin (per 1x incl. extras): %.3f", result_.expected_loss_per_spin);
		ImGui::BulletText("Lower risk or bet size for tighter stop.");
		ImGui::BulletText("Adjust risk or lock bet if target feels off.");
	}

	EndCard();
}