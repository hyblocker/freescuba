#pragma once

#include <openvr.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <d3d11.h>

#include "app_state.hpp"

namespace FreeScuba {
	namespace Overlay {

		// Functions to create and destroy an overlay for the window
		bool StartWindow();
		void DestroyWindow();
		bool UpdateNativeWindow(AppState& state, vr::VROverlayHandle_t overlayMainHandle);
	}
}