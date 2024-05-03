#pragma once

#include <imgui.h>
#include "imgui_extensions.hpp"
#include "app_state.hpp"

void SetupImgui();
void CleanupImgui();
void DrawUi(const bool isOverlay, AppState& state);

constexpr ImVec4 FINGER_CURL_COLOUR			= ImVec4(0.39f, 0.72f, 0.18f, 1.0f);
constexpr ImVec4 FINGER_REVERSE_CURL_COLOUR = ImVec4(0.8f, 0.21f, 0.21f, 1.0f);

inline void DRAW_FINGER_BEND_VALUE(const float fingerValue) {
	ImGui::ProgressBarPositiveNegative(fingerValue, FINGER_CURL_COLOUR, FINGER_REVERSE_CURL_COLOUR, ImVec2(-FLT_MIN, 0), "%.4f");
}

inline void DRAW_ALIGNED_SLIDER_FLOAT(const std::string label, float* v, const float v_min, const float v_max) {
    ImGui::TableSetColumnIndex(0);
    ImGui::PushFont(fontBold);
    ImGui::Text(label.c_str());
    ImGui::PopFont();
    ImGui::TableSetColumnIndex(1);
    ImGui::SliderFloat((std::string("##") + label).c_str(), v, v_min, v_max);
}