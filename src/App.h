#pragma once
#include "Models.h"
#include "Simulator.h"
#include "Style.h"
#include <imgui.h>
#include <string>
#include <vector>

class SlotPlannerApp {
public:
    SlotPlannerApp();
    void Draw();
private:
    std::vector<Game> games_;
    int game_idx_ = 0;
    SessionInput input_{};
    SimResult result_{};
    bool has_result_ = false;
    bool show_advanced_ = false;
    char custom_extra_name_[64] = "";
    float custom_extra_rtp_ = 0.95f;
    float custom_extra_cost_ = 0.50f;

    PathBands bands_{};
    bool bands_valid_ = false;   // cached result present?
    bool bands_dirty_ = true;    // need recompute?
    float bands_ymin_ = 0.f, bands_ymax_ = 0.f; // for axis lock

    void DrawLeftPane();
    void DrawRightPane();
    void DrawPlanSummary(const Game& g, const SessionInput& in, const SimResult& r);
};