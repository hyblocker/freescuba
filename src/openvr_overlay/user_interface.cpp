#include "user_interface.hpp"

void SetupImgui() {

}
void CleanupImgui() {

}

void DrawUi(bool isOverlay) {
	// @TODO: turn into imgui UI
	ImGui::ShowDemoWindow();

	auto& io = ImGui::GetIO();

	// Lock window to full screen
	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
	ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);

	ImGui::Begin("FreeScube - Open source contact gloves driver");

	ImGui::Text("Hello world!");

	ImGui::End();
}