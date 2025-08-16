#pragma once
#include <imgui.h>

inline void ApplyPrettyStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.FrameRounding = 10.0f;
    style.GrabRounding = 10.0f;
    style.WindowPadding = { 16,16 };
    style.FramePadding = { 12,8 };
    style.ItemSpacing = { 10,12 };
    style.ScrollbarRounding = 12.0f;

    auto& c = style.Colors;
    c[ImGuiCol_WindowBg] = { 0.05f,0.05f,0.06f,1.0f };
    c[ImGuiCol_TitleBg] = { 0.10f,0.10f,0.12f,1.0f };
    c[ImGuiCol_TitleBgActive] = { 0.12f,0.12f,0.16f,1.0f };
    c[ImGuiCol_PopupBg] = { 0.08f,0.08f,0.10f,1.0f };
    c[ImGuiCol_FrameBg] = { 0.12f,0.12f,0.14f,1.0f };
    c[ImGuiCol_FrameBgHovered] = { 0.18f,0.18f,0.22f,1.0f };
    c[ImGuiCol_FrameBgActive] = { 0.18f,0.18f,0.26f,1.0f };
    c[ImGuiCol_Button] = { 0.18f,0.22f,0.40f,1.0f };
    c[ImGuiCol_ButtonHovered] = { 0.22f,0.30f,0.55f,1.0f };
    c[ImGuiCol_ButtonActive] = { 0.16f,0.24f,0.50f,1.0f };
    c[ImGuiCol_Header] = { 0.18f,0.22f,0.40f,1.0f };
    c[ImGuiCol_HeaderHovered] = { 0.22f,0.30f,0.55f,1.0f };
    c[ImGuiCol_HeaderActive] = { 0.18f,0.22f,0.50f,1.0f };
}

inline void BeginCard(const char* title, ImVec2 size = { 0,0 }) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 14,12 });
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 18.0f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, { 0.10f,0.10f,0.12f,0.9f });
    ImGui::BeginChild(title, size, true, ImGuiWindowFlags_MenuBar);
    if (ImGui::BeginMenuBar()) { ImGui::TextUnformatted(title); ImGui::EndMenuBar(); }
}
inline void EndCard() { ImGui::EndChild(); ImGui::PopStyleColor(); ImGui::PopStyleVar(2); }