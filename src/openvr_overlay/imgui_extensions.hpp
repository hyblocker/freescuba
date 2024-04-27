#pragma once

#include <imgui.h>
#include <string>

namespace ImGui
{
	// Group panels taken from https://github.com/ocornut/imgui/issues/1496#issuecomment-655048353
	// Licensed under CC0 license as per https://github.com/ocornut/imgui/issues/1496#issuecomment-1287772456

	void BeginGroupPanel(const char* name, const ImVec2& size = ImVec2(0.0f, 0.0f));
	void EndGroupPanel();
	ImVec2 GetCursorPosAbsolute();

	void ProgressBarPositiveNegative(float fraction, ImVec4 positive, ImVec4 negative, const ImVec2& size_arg, const char* overlay);

	bool InputDouble3(const char* label, double v[3], const char* format, ImGuiInputTextFlags flags = 0);

	void DrawVectorElement(std::string id, const char* text, double* value, double delta = 0.01);
}