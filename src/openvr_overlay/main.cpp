#include "overlay_app.hpp"

int main() {
	if (FreeScuba::Overlay::StartWindow()) {
	bool doExecute = true;
		while (doExecute) {
			doExecute = FreeScuba::Overlay::UpdateNativeWindow();
		}

		FreeScuba::Overlay::DestroyWindow();
	}
}