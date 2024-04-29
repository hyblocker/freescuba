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