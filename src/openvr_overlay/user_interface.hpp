#pragma once

#include <imgui.h>
#include "app_state.hpp"

void SetupImgui();
void CleanupImgui();
void DrawUi(bool isOverlay, AppState& state);