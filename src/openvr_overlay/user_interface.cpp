#define _USE_MATH_DEFINES
#include <openvr.h>

#include "user_interface.hpp"

#include "maths.hpp"

// DrawJoystickInput :: Imgui commands to draw a nice joystick control to show what the current joystick value is
// DrawGlove :: Displays a single Glove's state. Contains the buttons for trigger state change.

ImFont* fontBold    = nullptr;
ImFont* fontLight   = nullptr;
ImFont* fontRegular = nullptr;

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define CLAMP(t,a,b) (MAX(MIN(t, b), a))

void SetupImgui() {
    // @TODO: ImGui style here

    ImGuiIO& io = ImGui::GetIO();

    ImGuiStyle& style = ImGui::GetStyle();

    // Main window should not have any border as it's rooted to the executable window
    style.WindowBorderSize  = 0.0f;
    style.WindowRounding    = 0.0f;

    // Rounding
    style.ChildRounding     = 6.0f;
    style.FrameRounding     = 6.0f;
    style.PopupRounding     = 6.0f;
    style.TabRounding       = 6.0f;

    // Colours
    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_WindowBg]           = ImVec4(0.01f, 0.01f, 0.01f, 0.94f);
    colors[ImGuiCol_Border]             = ImVec4(0.30f, 0.30f, 0.30f, 0.55f);
    colors[ImGuiCol_FrameBg]            = ImVec4(0.24f, 0.26f, 0.27f, 0.54f);
    colors[ImGuiCol_FrameBgHovered]     = ImVec4(0.26f, 0.26f, 0.26f, 0.40f);
    colors[ImGuiCol_FrameBgActive]      = ImVec4(0.31f, 0.31f, 0.31f, 0.67f);
    colors[ImGuiCol_SliderGrab]         = ImVec4(0.26f, 0.43f, 0.68f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]   = ImVec4(0.36f, 0.56f, 0.86f, 1.00f);
    colors[ImGuiCol_Button]             = ImVec4(0.26f, 0.43f, 0.68f, 1.00f);
    colors[ImGuiCol_ButtonHovered]      = ImVec4(0.29f, 0.52f, 0.85f, 1.00f);
    colors[ImGuiCol_ButtonActive]       = ImVec4(0.21f, 0.33f, 0.52f, 1.00f);

    const float FONT_SIZE = 16.0f;

    fontRegular = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf",  FONT_SIZE); // Segoe UI
    fontBold    = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeuib.ttf", FONT_SIZE); // Segoe UI Bold
    fontLight   = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeuil.ttf", FONT_SIZE); // Segoe UI Light
}
void CleanupImgui() {

}

void DrawJoystickInput(const float valueX, const float valueY, const float deadzone) {
    const float deadzoneRemapped = deadzone * 100.f;
    const ImU32 whiteColor = IM_COL32(255, 255, 255, 255);
    constexpr float BOX_SIZE = 200;
    constexpr float CIRCLE_RADIUS = 10;
    const ImVec2 posR = ImGui::GetCursorPosAbsolute();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Safe region
    drawList->AddRectFilled(posR, ImVec2(posR.x + BOX_SIZE, posR.y + BOX_SIZE), IM_COL32(30,0,0,255));
    drawList->AddCircleFilled(ImVec2(posR.x + BOX_SIZE * 0.5f, posR.y + BOX_SIZE * 0.5f), deadzoneRemapped, IM_COL32(0,0,0,255), 32);

    // Frame and cross lines
    drawList->AddRect(posR, ImVec2(posR.x + 200, posR.y + 200), whiteColor);
    drawList->AddLine(ImVec2(posR.x + 100, posR.y), ImVec2(posR.x + 100, posR.y + 200), whiteColor);
    drawList->AddLine(ImVec2(posR.x, posR.y + 100), ImVec2(posR.x + 200, posR.y + 100), whiteColor);
    // Draw threshold and unit radius circles.
    drawList->AddCircle(ImVec2(posR.x + 100, posR.y + 100), deadzoneRemapped, IM_COL32(0, 255, 0, 255), 32);
    drawList->AddCircle(ImVec2(posR.x + 100, posR.y + 100), 100, whiteColor, 32);
    // Current axis position.
    drawList->AddCircleFilled(ImVec2(posR.x + valueX * 100 + 100, posR.y + valueY * 100 + 100), CIRCLE_RADIUS, IM_COL32(255, 255, 255, 191));
    drawList->AddCircleFilled(ImVec2(posR.x + valueX * 100 + 100, posR.y + valueY * 100 + 100), 1.1f, IM_COL32(255, 0, 0, 255));

    ImGui::Dummy(ImVec2(BOX_SIZE, BOX_SIZE));
}

void DrawGlove(const std::string name, const std::string id, protocol::ContactGloveState_t& glove, AppState& state) {

    std::string panelTitle = name;
    if (glove.isConnected) {
        panelTitle = name + " (" + std::to_string(CLAMP(glove.gloveBattery, 0, 100)) + "%)";
    }
    ImGui::BeginGroupPanel(panelTitle.c_str());
    {
        if (!glove.isConnected) {
            ImGui::TextDisabled("Glove not connected...");
        } else {
            ImGui::PushID((id + "_magnetra_group").c_str());
            ImGui::BeginGroupPanel("Magnetra Settings");
            {
                ImGui::Spacing();
                ImGui::Spacing();

                if (!glove.hasMagnetra) {
                    ImGui::TextDisabled("Magnetra is not available... Inputs unavailable.");
                } else {
                    ImGui::PushFont(fontBold);
                    ImGui::Text("Joystick");
                    ImGui::PopFont();
                    ImGui::SameLine();
                    ImGui::Text(": (%.4f, %.4f)", glove.joystickX, glove.joystickY);
                    ImGui::Spacing();
                    DrawJoystickInput(glove.joystickXUnfiltered, glove.joystickYUnfiltered, glove.calibration.joystick.threshold);

                    ImGui::Spacing();
                    ImGui::Spacing();

                    ImGui::Text("Deadzone");
                    ImGui::SameLine();
                    ImGui::PushID((id + "_joystick_threshold").c_str());
                    ImGui::SliderFloat("##threshold", &glove.calibration.joystick.threshold, 0.f, 1.0f);
                    ImGui::PopID();

                    ImGui::Spacing();
                    ImGui::Spacing();

                    ImGui::PushFont(fontBold);
                    ImGui::Text("Buttons");
                    ImGui::PopFont();

                    float columnWidth = ImGui::CalcItemWidth() / 5.0f;
                    ImGui::PushID((id + "_buttons_container").c_str());
                    ImGui::Columns(5);
                    for (int i = 0; i < 5; i++) {
                        ImGui::SetColumnWidth(i, columnWidth);
                    }
                    ImGui::PushID((id + "_button_a").c_str());
                    ImGui::RadioButton("A##gloveL", glove.buttonUp);
                    ImGui::PopID();
                    ImGui::NextColumn();
                    ImGui::PushID((id + "_button_b").c_str());
                    ImGui::RadioButton("B##gloveL", glove.buttonDown);
                    ImGui::PopID();
                    ImGui::NextColumn();
                    ImGui::PushID((id + "_system_up").c_str());
                    ImGui::RadioButton("Sys Up##gloveL", glove.systemUp);
                    ImGui::PopID();
                    ImGui::NextColumn();
                    ImGui::PushID((id + "_system_down").c_str());
                    ImGui::RadioButton("Sys Down##gloveL", glove.systemDown);
                    ImGui::PopID();
                    ImGui::NextColumn();
                    ImGui::PushID((id + "_joystick_click").c_str());
                    ImGui::RadioButton("Joystick Click##gloveL", glove.joystickClick);
                    ImGui::PopID();
                    ImGui::Columns(1);
                    ImGui::PopID();

                    ImGui::Spacing();
                    ImGui::Spacing();

                    if (ImGui::Button("Calibrate Thumbstick")) {
                        state.uiState.page = ScreenState_t::ScreenStateCalibrateJoystick;
                        state.uiState.calibrationState = CalibrationState_t::State_Entering;
                    }

                    // @TODO: Adjust forward angle for fine tuning
                }

                ImGui::Spacing();
            }
            ImGui::EndGroupPanel();
            ImGui::PopID();

            ImGui::PushID((id + "_fingers_group").c_str());
            ImGui::BeginGroupPanel("Finger Tracking");
            {
                ImGui::Spacing();
                ImGui::Spacing();
                if (ImGui::BeginTable((id + "_fingers_table").c_str(), 4, ImGuiTableFlags_SizingFixedFit))
                {
                    float windowWidth = ImGui::GetWindowWidth();
                    ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 0.0f);
                    ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 0.0f);
                    float widthBars = (windowWidth - ImGui::GetColumnWidth(0) - ImGui::GetColumnWidth(1)) * 0.5f;
                    ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoResize, widthBars);
                    ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoResize, widthBars);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::PushFont(fontBold);
                    ImGui::Text("Thumb");
                    ImGui::PopFont();
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("(%.4f, %.4f)", glove.thumbRoot, glove.thumbTip);
                    ImGui::TableSetColumnIndex(2);
                    DRAW_FINGER_BEND_VALUE(glove.thumbRoot);
                    ImGui::TableSetColumnIndex(3);
                    DRAW_FINGER_BEND_VALUE(glove.thumbTip);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::PushFont(fontBold);
                    ImGui::Text("Index finger");
                    ImGui::PopFont();
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("(%.4f, %.4f)", glove.indexRoot, glove.indexTip);
                    ImGui::TableSetColumnIndex(2);
                    DRAW_FINGER_BEND_VALUE(glove.indexRoot);
                    ImGui::TableSetColumnIndex(3);
                    DRAW_FINGER_BEND_VALUE(glove.indexTip);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::PushFont(fontBold);
                    ImGui::Text("Middle finger");
                    ImGui::PopFont();
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("(%.4f, %.4f)", glove.middleRoot, glove.middleTip);
                    ImGui::TableSetColumnIndex(2);
                    DRAW_FINGER_BEND_VALUE(glove.middleRoot);
                    ImGui::TableSetColumnIndex(3);
                    DRAW_FINGER_BEND_VALUE(glove.middleTip);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::PushFont(fontBold);
                    ImGui::Text("Ring finger");
                    ImGui::PopFont();
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("(%.4f, %.4f)", glove.ringRoot, glove.ringTip);
                    ImGui::TableSetColumnIndex(2);
                    DRAW_FINGER_BEND_VALUE(glove.ringRoot);
                    ImGui::TableSetColumnIndex(3);
                    DRAW_FINGER_BEND_VALUE(glove.ringTip);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::PushFont(fontBold);
                    ImGui::Text("Pinky finger");
                    ImGui::PopFont();
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("(%.4f, %.4f)", glove.pinkyRoot, glove.pinkyTip);
                    ImGui::TableSetColumnIndex(2);
                    DRAW_FINGER_BEND_VALUE(glove.pinkyRoot);
                    ImGui::TableSetColumnIndex(3);
                    DRAW_FINGER_BEND_VALUE(glove.pinkyTip);

                    ImGui::EndTable();
                }

                ImGui::Spacing(); 
                ImGui::Spacing();

                if (ImGui::Button("Calibrate fingers")) {
                    state.uiState.page = ScreenState_t::ScreenStateCalibrateFingers;
                    state.uiState.calibrationState = CalibrationState_t::State_Entering;
                }

                ImGui::Checkbox("Use finger curl", &glove.useCurl);

                ImGui::Spacing();

                ImGui::PushID((id + "_fingers_gestures_group").c_str());
                ImGui::BeginGroupPanel("Gestures");
                {
                    ImGui::Spacing();
                    ImGui::Spacing();

                    if (ImGui::BeginTable((id + "_fingers_slider_group").c_str(), 3, ImGuiTableFlags_SizingFixedFit)) {
                        ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 0.0f);   
                        ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoResize, 0.0f);

                        ImGui::TableNextRow();
                        DRAW_ALIGNED_SLIDER_FLOAT("Thumb Activate", &glove.calibration.gestures.thumb.activate, 0, 1);
                        ImGui::TableNextRow();
                        DRAW_ALIGNED_SLIDER_FLOAT("Thumb Deactivate", &glove.calibration.gestures.thumb.deactivate, 0, 1);
                        ImGui::TableNextRow();

                        DRAW_ALIGNED_SLIDER_FLOAT("Trigger Activate", &glove.calibration.gestures.trigger.activate, 0, 1);
                        ImGui::TableNextRow();
                        DRAW_ALIGNED_SLIDER_FLOAT("Trigger Deactivate", &glove.calibration.gestures.trigger.deactivate, 0, 1);
                        ImGui::TableNextRow();

                        DRAW_ALIGNED_SLIDER_FLOAT("Grip Activate", &glove.calibration.gestures.grip.activate, 0, 1);
                        ImGui::TableNextRow();
                        DRAW_ALIGNED_SLIDER_FLOAT("Grip Deactivate", &glove.calibration.gestures.grip.deactivate, 0, 1);

                        ImGui::EndTable();
                    }

                    ImGui::Spacing();
                }
                
                if (ImGui::CollapsingHeader("Raw values")) {
                    ImGui::Text("Thumb Root:  %d", glove.thumbRootRaw);
                    ImGui::Text("Thumb Tip:   %d", glove.thumbTipRaw);
                    ImGui::Text("Index Root:  %d", glove.indexRootRaw);
                    ImGui::Text("Index Tip:   %d", glove.indexTipRaw);
                    ImGui::Text("Middle Root: %d", glove.middleRootRaw);
                    ImGui::Text("Middle Tip:  %d", glove.middleTipRaw);
                    ImGui::Text("Ring Root:   %d", glove.ringRootRaw);
                    ImGui::Text("Ring Tip:    %d", glove.ringTipRaw);
                    ImGui::Text("Pinky Root:  %d", glove.pinkyRootRaw);
                    ImGui::Text("Pinky Tip:   %d", glove.pinkyTipRaw);
                }

                ImGui::EndGroupPanel();
                ImGui::PopID();
            }
            ImGui::EndGroupPanel();
            ImGui::PopID();

            ImGui::PushFont(fontBold);
            ImGui::Text("Misc");
            ImGui::PopFont();
            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::TextDisabled("Battery: ");
            ImGui::SameLine();
            ImGui::Text((std::to_string(glove.gloveBattery) + "%%").c_str());
            ImGui::SameLine();
            ImGui::TextDisabled("    Version: ");
            ImGui::SameLine();
            ImGui::Text((std::to_string(glove.firmwareMajor) + "." + std::to_string(glove.firmwareMinor)).c_str());

            {
                ImGui::PushID((id + "_offset_adjust_group").c_str());
                // Disallow tracker calibration if a tracker is not available
                ImGui::BeginDisabled(glove.trackerIndex == CONTACT_GLOVE_INVALID_DEVICE_ID);

                if (ImGui::Button("Calibrate offset")) {
                    state.uiState.page = ScreenState_t::ScreenStateCalibrateOffset;
                    state.uiState.calibrationState = CalibrationState_t::State_Entering;
                }

                ImGui::SameLine();

                if (ImGui::Button("Reset offset")) {
                    glove.calibration.poseOffset.pos.v[0] = 0;
                    glove.calibration.poseOffset.pos.v[1] = 0;
                    glove.calibration.poseOffset.pos.v[2] = 0;

                    glove.calibration.poseOffset.rot.x = 0.0f;
                    glove.calibration.poseOffset.rot.y = 0.0f;
                    glove.calibration.poseOffset.rot.z = 0.0f;
                    glove.calibration.poseOffset.rot.w = 1.0f;
                }

                ImGui::EndDisabled();

                ImGui::Spacing();
                ImGui::PushFont(fontBold);
                ImGui::Text("Position Offset");
                ImGui::PopFont();
                ImGui::Spacing();

                ImGui::DrawVectorElement(id + "_tracker_position_offset", "X", &glove.calibration.poseOffset.pos.v[0]);
                ImGui::DrawVectorElement(id + "_tracker_position_offset", "Y", &glove.calibration.poseOffset.pos.v[1]);
                ImGui::DrawVectorElement(id + "_tracker_position_offset", "Z", &glove.calibration.poseOffset.pos.v[2]);

                ImGui::PopID();
            }
            ImGui::Spacing();
        }
    }
    ImGui::EndGroupPanel();
}

void DrawCalibrateJoystick(AppState& state) {
    ImGui::BeginGroupPanel("Joystick Calibration");
    {
        // @TODO: Style the UI to look pretty

        // Isolate the glove we wish to work on
        protocol::ContactGloveState_t* desiredGlove = nullptr;
        if (state.uiState.processingHandedness == Handedness_t::Left) {
            ImGui::Text("Calibrating Left Joystick...");
            desiredGlove = &state.gloveLeft;
        }
        else {
            ImGui::Text("Calibrating Right Joystick...");
            desiredGlove = &state.gloveRight;
        }

        // Handled here so that we don't get a blank frame
        if (state.uiState.calibrationState == CalibrationState_t::State_Entering) {
            // Copy the old calibration data
            memcpy(&state.uiState.oldCalibration, &desiredGlove->calibration, sizeof(protocol::ContactGloveState_t::CalibrationData_t));
            // Create a new calibration state, and 0 the joystick calibration
            memcpy(&state.uiState.currentCalibration, &desiredGlove->calibration, sizeof(protocol::ContactGloveState_t::CalibrationData_t));
            memset(&state.uiState.currentCalibration.joystick, 0, sizeof(protocol::ContactGloveState_t::CalibrationData_t::JoystickCalibration_t));
            
            // Set intial values for calibration
            state.uiState.currentCalibration.joystick.XMax		= 0x0000;
            state.uiState.currentCalibration.joystick.XMin		= 0xFFFF;
            state.uiState.currentCalibration.joystick.YMax		= 0x0000;
            state.uiState.currentCalibration.joystick.YMin		= 0xFFFF;
            state.uiState.currentCalibration.joystick.XNeutral	= 0x0000;
            state.uiState.currentCalibration.joystick.YNeutral	= 0xFFFF;
            state.uiState.joystickForwardX						= 0x0000;
            state.uiState.joystickForwardY						= 0x0000;
            state.uiState.currentCalibration.joystick.forwardAngle		= 0.0f;

            // Copy user settings
            state.uiState.currentCalibration.joystick.threshold = desiredGlove->calibration.joystick.threshold;

            // Done. Move to DiscoverBounds
            state.uiState.calibrationState = CalibrationState_t::Joystick_DiscoverBounds;
        }

        // Cache if the buttons are pressed
        const bool anyButtonPressedNoJoystick =
            state.uiState.gloveButtons.releasedLeft.buttonUp || state.uiState.gloveButtons.releasedLeft.buttonDown || state.uiState.gloveButtons.releasedLeft.systemUp || state.uiState.gloveButtons.releasedLeft.systemDown ||
            state.uiState.gloveButtons.releasedRight.buttonUp || state.uiState.gloveButtons.releasedRight.buttonDown || state.uiState.gloveButtons.releasedRight.systemUp || state.uiState.gloveButtons.releasedRight.systemDown;
        
        // Calibration code
        switch (state.uiState.calibrationState) {
            case CalibrationState_t::Joystick_DiscoverBounds:
                {
                    ImGui::Text("Move the joystick in all directions for a few seconds...");

                    // Discover min max for each joystick
                    state.uiState.currentCalibration.joystick.XMax = MAX(state.uiState.currentCalibration.joystick.XMax, desiredGlove->joystickXRaw);
                    state.uiState.currentCalibration.joystick.XMin = MIN(state.uiState.currentCalibration.joystick.XMin, desiredGlove->joystickXRaw);

                    state.uiState.currentCalibration.joystick.YMax = MAX(state.uiState.currentCalibration.joystick.YMax, desiredGlove->joystickYRaw);
                    state.uiState.currentCalibration.joystick.YMin = MIN(state.uiState.currentCalibration.joystick.YMin, desiredGlove->joystickYRaw);

                    // Proceed on any input
                    ImGui::Text("Press any button to continue...");

                    if (ImGui::Button("Continue") || anyButtonPressedNoJoystick) {
                        // Move to the next state
                        state.uiState.calibrationState = CalibrationState_t::Joystick_DiscoverNeutral;
                    }

                    break;
                }
            case CalibrationState_t::Joystick_DiscoverNeutral:
                {
                    // Discover neutral for joystick
                    ImGui::Text("Do not move the joystick, let it settle.");

                    state.uiState.currentCalibration.joystick.XNeutral = desiredGlove->joystickXRaw;
                    state.uiState.currentCalibration.joystick.YNeutral = desiredGlove->joystickYRaw;

                    // Proceed on any input
                    ImGui::Text("Press any button to continue...");

                    if (ImGui::Button("Continue") || anyButtonPressedNoJoystick) {
                        // Move to the next state
                        state.uiState.calibrationState = CalibrationState_t::Joystick_DiscoverForward;
                    }

                    break;
                }
            case CalibrationState_t::Joystick_DiscoverForward:
                {
                    // Discover the forward direction of the joystick, compute an output angle from this
                    ImGui::Text("Move the joystick forward.");

                    state.uiState.joystickForwardX = desiredGlove->joystickXRaw;
                    state.uiState.joystickForwardY = desiredGlove->joystickYRaw;

                    // Proceed on any input
                    ImGui::Text("Press any button to continue...");

                    if (ImGui::Button("Continue") || anyButtonPressedNoJoystick) {
                        // Compute the forward vector
                        float joystickX = 2.0f * (state.uiState.joystickForwardX - state.uiState.currentCalibration.joystick.XMin) / (float)MAX(state.uiState.currentCalibration.joystick.XMax - state.uiState.currentCalibration.joystick.XMin, 0.0f) - 1.0f;
                        float joystickY = 2.0f * (state.uiState.joystickForwardY - state.uiState.currentCalibration.joystick.YMin) / (float)MAX(state.uiState.currentCalibration.joystick.YMax - state.uiState.currentCalibration.joystick.YMin, 0.0f) - 1.0f;

                        // Normalize the axis if out of range
                        if (joystickX * joystickX + joystickY * joystickY > 1.0f) {
                            const double length = sqrt(joystickX * joystickX + joystickY * joystickY);
                            joystickX = (float) (joystickX / length);
                            joystickY = (float) (joystickY / length);
                        }

                        // Get the angle it's at, and store in the calibration
                        const float angle = atan2(joystickX , -joystickY);
                        state.uiState.currentCalibration.joystick.forwardAngle = angle; // Negate the angle, and rotate -90 degrees

                        // Move to the next state
                        state.uiState.calibrationState = CalibrationState_t::State_Entering;
                        state.uiState.page = ScreenState_t::ScreenStateViewData;

                        // Copy the new calibration back
                        memcpy(&desiredGlove->calibration , &state.uiState.currentCalibration, sizeof(protocol::ContactGloveState_t::CalibrationData_t));
                    }

                    break;
                }
            default:
                {
                    ImGui::Text((std::string("Encountered broken state! (") + std::to_string((int)state.uiState.calibrationState) + ")").c_str());
                    break;
                }
        }

        if (ImGui::Button("Cancel")) {
            // Move to the data page
            state.uiState.calibrationState = CalibrationState_t::State_Entering;
            state.uiState.page = ScreenState_t::ScreenStateViewData;

            // Set the old calibration
            // Should not be necessary
            // memcpy(&desiredGlove->calibration, &state.uiState.oldCalibration, sizeof(protocol::ContactGloveState::CalibrationData));
        }
    }
    ImGui::EndGroupPanel();
}

void DrawCalibrateFingers(AppState& state) {

    ImGui::BeginGroupPanel("Finger Calibration");
    {
        protocol::ContactGloveState_t* desiredGlove = nullptr;
        if (state.uiState.processingHandedness == Handedness_t::Left) {
            ImGui::Text("Calibrating Left Glove Fingers...");
            desiredGlove = &state.gloveLeft;
        }
        else {
            ImGui::Text("Calibrating Right Glove Fingers...");
            desiredGlove = &state.gloveRight;
        }

        // Handled here so that we don't get a blank frame
        if (state.uiState.calibrationState == CalibrationState_t::State_Entering) {
            // Copy the old calibration data
            memcpy(&state.uiState.oldCalibration, &desiredGlove->calibration, sizeof(protocol::ContactGloveState_t::CalibrationData_t));

            // Create a new calibration state, and 0 out the finger calibration
            memcpy(&state.uiState.currentCalibration, &desiredGlove->calibration, sizeof(protocol::ContactGloveState_t::CalibrationData_t));
            memset(&state.uiState.currentCalibration.fingers, 0, sizeof(protocol::ContactGloveState_t::FingerCalibrationData_t));
            
            // Done. Move to DiscoverBounds
            state.uiState.calibrationState = CalibrationState_t::Fingers_DiscoverNeutral;
        }

        // Cache if the buttons are pressed
        const bool anyButtonPressedJoystick =
            state.uiState.gloveButtons.releasedLeft.buttonUp || state.uiState.gloveButtons.releasedLeft.buttonDown || state.uiState.gloveButtons.releasedLeft.systemUp || state.uiState.gloveButtons.releasedLeft.systemDown || state.uiState.gloveButtons.releasedLeft.joystickClick ||
            state.uiState.gloveButtons.releasedRight.buttonUp || state.uiState.gloveButtons.releasedRight.buttonDown || state.uiState.gloveButtons.releasedRight.systemUp || state.uiState.gloveButtons.releasedRight.systemDown || state.uiState.gloveButtons.releasedRight.joystickClick;

        // Calibration code
        switch (state.uiState.calibrationState) {
            case CalibrationState_t::Fingers_DiscoverNeutral:
            {
                // Fingers are open, thumb is closed
                ImGui::Text("Rest your fingers naturally, and close your thumb. Try aligning the fingers with the way you see them in VRChat for better accuracy.");

                state.uiState.currentCalibration.fingers.thumbRoot.close	= desiredGlove->thumbRootRaw;
                state.uiState.currentCalibration.fingers.thumbTip.close		= desiredGlove->thumbTipRaw;

                state.uiState.currentCalibration.fingers.indexRoot.rest		= desiredGlove->indexRootRaw;
                state.uiState.currentCalibration.fingers.indexTip.rest		= desiredGlove->indexTipRaw;

                state.uiState.currentCalibration.fingers.middleRoot.rest	= desiredGlove->middleRootRaw;
                state.uiState.currentCalibration.fingers.middleTip.rest		= desiredGlove->middleTipRaw;

                state.uiState.currentCalibration.fingers.ringRoot.rest		= desiredGlove->ringRootRaw;
                state.uiState.currentCalibration.fingers.ringTip.rest		= desiredGlove->ringTipRaw;

                state.uiState.currentCalibration.fingers.pinkyRoot.rest		= desiredGlove->pinkyRootRaw;
                state.uiState.currentCalibration.fingers.pinkyTip.rest		= desiredGlove->pinkyTipRaw;

                // Override the fingers because we're mid-calibration
                desiredGlove->thumbRoot     = 1;
                desiredGlove->thumbTip      = 1;
                desiredGlove->indexRoot     = 0;
                desiredGlove->indexTip      = 0;
                desiredGlove->middleRoot    = 0;
                desiredGlove->middleTip     = 0;
                desiredGlove->ringRoot      = 0;
                desiredGlove->ringTip       = 0;
                desiredGlove->pinkyRoot     = 0;
                desiredGlove->pinkyTip      = 0;

                // Proceed on any input
                ImGui::Text("Press any button to continue...");

                if (ImGui::Button("Continue") || anyButtonPressedJoystick) {
                    // Move to the next state
                    state.uiState.calibrationState = CalibrationState_t::Fingers_DiscoverClosed;
                }

                break;
            }
            case CalibrationState_t::Fingers_DiscoverClosed:
            {
                // Fingers are closed, thumb is expanded
                ImGui::Text("Close your fingers, and rest your thumb naturally. Try aligning the fingers with the way you see them in VRChat for better accuracy.");

                state.uiState.currentCalibration.fingers.thumbRoot.rest		= desiredGlove->thumbRootRaw;
                state.uiState.currentCalibration.fingers.thumbTip.rest		= desiredGlove->thumbTipRaw;

                state.uiState.currentCalibration.fingers.indexRoot.close	= desiredGlove->indexRootRaw;
                state.uiState.currentCalibration.fingers.indexTip.close		= desiredGlove->indexTipRaw;

                state.uiState.currentCalibration.fingers.middleRoot.close	= desiredGlove->middleRootRaw;
                state.uiState.currentCalibration.fingers.middleTip.close	= desiredGlove->middleTipRaw;

                state.uiState.currentCalibration.fingers.ringRoot.close		= desiredGlove->ringRootRaw;
                state.uiState.currentCalibration.fingers.ringTip.close		= desiredGlove->ringTipRaw;

                state.uiState.currentCalibration.fingers.pinkyRoot.close	= desiredGlove->pinkyRootRaw;
                state.uiState.currentCalibration.fingers.pinkyTip.close		= desiredGlove->pinkyTipRaw;

                // Override the fingers because we're mid-calibration
                desiredGlove->thumbRoot     = 0;
                desiredGlove->thumbTip      = 0;
                desiredGlove->indexRoot     = 1;
                desiredGlove->indexTip      = 1;
                desiredGlove->middleRoot    = 1;
                desiredGlove->middleTip     = 1;
                desiredGlove->ringRoot      = 1;
                desiredGlove->ringTip       = 1;
                desiredGlove->pinkyRoot     = 1;
                desiredGlove->pinkyTip      = 1;

                // Proceed on any input
                ImGui::Text("Press any button to continue...");

                if (ImGui::Button("Continue") || anyButtonPressedJoystick) {
                    // Move to the next state
                    // state.uiState.calibrationState = CalibrationState::Fingers_DiscoverClosed;

                    // Move to the next state
                    state.uiState.calibrationState = CalibrationState_t::State_Entering;
                    state.uiState.page = ScreenState_t::ScreenStateViewData;

                    // Copy the new calibration back
                    memcpy(&desiredGlove->calibration, &state.uiState.currentCalibration, sizeof(protocol::ContactGloveState_t::CalibrationData_t));
                }

                break;
            }
            case CalibrationState_t::Fingers_DiscoverBackwardsBend:
            {
                // Maybe unecessary, with linear interpolation this should be a given

                break;
            }
            default:
            {
                ImGui::Text((std::string("Encountered broken state! (") + std::to_string((int)state.uiState.calibrationState) + ")").c_str());
                break;
            }
        }

        if (ImGui::Button("Cancel")) {
            // Move to the data page
            state.uiState.calibrationState = CalibrationState_t::State_Entering;
            state.uiState.page = ScreenState_t::ScreenStateViewData;

            // Set the old calibration
            // Should not be necessary

        }
    }
    ImGui::EndGroupPanel();
}

void DrawCalibrateOffsets(AppState& state) {

    ImGui::BeginGroupPanel("Pose Offset Calibration");
    {
        protocol::ContactGloveState_t* desiredGlove = nullptr;
        if (state.uiState.processingHandedness == Handedness_t::Left) {
            ImGui::Text("Calibrating Left Glove Pose Offset...");
            desiredGlove = &state.gloveLeft;
        } else {
            ImGui::Text("Calibrating Right Glove Pose Offset...");
            desiredGlove = &state.gloveRight;
        }

        // Handled here so that we don't get a blank frame
        if (state.uiState.calibrationState == CalibrationState_t::State_Entering) {
            // Copy the old calibration data
            memcpy(&state.uiState.oldCalibration, &desiredGlove->calibration, sizeof(protocol::ContactGloveState_t::CalibrationData_t));

            // Create a new calibration state, and 0 out the pose calibration
            memcpy(&state.uiState.currentCalibration, &desiredGlove->calibration, sizeof(protocol::ContactGloveState_t::CalibrationData_t));
            memset(&state.uiState.currentCalibration.poseOffset, 0, sizeof(protocol::ContactGloveState_t::CalibrationData_t::PoseOffset_t));

            // Done. Move to DiscoverBounds
            state.uiState.calibrationState = CalibrationState_t::PoseOffset_FindInitialPose;
        }

        // Cache if the buttons are pressed
        const bool anyButtonPressedJoystick =
            state.uiState.gloveButtons.releasedLeft.buttonUp || state.uiState.gloveButtons.releasedLeft.buttonDown || state.uiState.gloveButtons.releasedLeft.systemUp || state.uiState.gloveButtons.releasedLeft.systemDown || state.uiState.gloveButtons.releasedLeft.joystickClick ||
            state.uiState.gloveButtons.releasedRight.buttonUp || state.uiState.gloveButtons.releasedRight.buttonDown || state.uiState.gloveButtons.releasedRight.systemUp || state.uiState.gloveButtons.releasedRight.systemDown || state.uiState.gloveButtons.releasedRight.joystickClick;

        // Calibration code
        switch (state.uiState.calibrationState) {
            case CalibrationState_t::PoseOffset_FindInitialPose:
            {
                ImGui::Text("Waiting for a valid pose. Please make sure your tracker is on, and tracking.");
                
                // Request a driver pose from the server
                protocol::Request_t req = protocol::Request_t(desiredGlove->trackerIndex);
                protocol::Response_t response = state.ipcClient->SendBlocking(req);

                if (response.type == protocol::ResponseDevicePose) {

                    vr::DriverPose_t driverPose = response.driverPose;

                    const vr::HmdVector3d_t driverPosition = { driverPose.vecPosition[0], driverPose.vecPosition[1], driverPose.vecPosition[2] };
                    state.uiState.initialTrackerPos = driverPosition + (desiredGlove->calibration.poseOffset.pos * driverPose.qRotation);
                    state.uiState.initialTrackerRot = driverPose.qRotation * desiredGlove->calibration.poseOffset.rot;

                    // Move to the next state
                    state.uiState.calibrationState = CalibrationState_t::PoseOffset_Moving;

                    // Tell the driver to ignore pose updates, we shall monitor the device's transform in the meantime
                    desiredGlove->ignorePose = true;
                }

                break;
            }
        case CalibrationState_t::PoseOffset_Moving:
        {
            ImGui::Text("Line up the glove with your hand. Once you find the right spot, press any button to assign the offset.");

            // Tell the driver to ignore pose updates, we shall monitor the device's transform in the meantime
            desiredGlove->ignorePose = true;

            // Proceed on any input
            ImGui::Text("Press any button to continue...");

            if (ImGui::Button("Continue") || anyButtonPressedJoystick) {

                // Request a driver pose from the server
                protocol::Request_t req = protocol::Request_t(desiredGlove->trackerIndex);
                protocol::Response_t response = state.ipcClient->SendBlocking(req);

                if (response.type == protocol::ResponseDevicePose) {

                    vr::DriverPose_t driverPose = response.driverPose;

                    const vr::HmdVector3d_t driverPosition = { driverPose.vecPosition[0], driverPose.vecPosition[1], driverPose.vecPosition[2] };
                    const vr::HmdVector3d_t newPos = driverPosition + (desiredGlove->calibration.poseOffset.pos * driverPose.qRotation);
                    const vr::HmdQuaternion_t newRot = driverPose.qRotation * desiredGlove->calibration.poseOffset.rot;

                    // Compute the deltas...
                    // Maths taken from openglove driver
                    const vr::HmdQuaternion_t transformQuat = -newRot * state.uiState.initialTrackerRot;

                    const vr::HmdVector3d_t differenceVector = state.uiState.initialTrackerPos - newPos;
                    const vr::HmdVector3d_t transformVector = differenceVector * -newRot;

                    state.uiState.currentCalibration.poseOffset.rot = transformQuat;
                    state.uiState.currentCalibration.poseOffset.pos = transformVector;

                    // Tell the driver to stop ignoring pose updates, we have an offset now!
                    desiredGlove->ignorePose = false;

                    // Copy the new calibration back
                    memcpy(&desiredGlove->calibration, &state.uiState.currentCalibration, sizeof(protocol::ContactGloveState_t::CalibrationData_t));

                    // Move to the next state
                    state.uiState.calibrationState = CalibrationState_t::State_Entering;
                    state.uiState.page = ScreenState_t::ScreenStateViewData;
                }
            }

            break;
        }
            // Maybe unecessary, with linear interpolation this should be a given
        default:
        {
            ImGui::Text((std::string("Encountered broken state! (") + std::to_string((int)state.uiState.calibrationState) + ")").c_str());
            break;
        }
        }

        if (ImGui::Button("Cancel")) {
            // Tell the driver to stop ignoring pose updates, we have decided to stop calibrating the offset
            desiredGlove->ignorePose = false;

            // Move to the data page
            state.uiState.calibrationState = CalibrationState_t::State_Entering;
            state.uiState.page = ScreenState_t::ScreenStateViewData;
        }
    }
    ImGui::EndGroupPanel();
}

void DrawUi(const bool isOverlay, AppState& state) {
#ifdef _DEBUG
    ImGui::ShowDemoWindow();
#endif

    const ImGuiIO& io = ImGui::GetIO();

    // Lock window to full screen
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);

    ImGui::Begin("FreeScuba - Open source contact gloves driver", 0, ImGuiWindowFlags_NoTitleBar);

    switch (state.uiState.page) {
        case ScreenState_t::ScreenStateViewData: {
            if (state.uiState.page == ScreenState_t::ScreenStateViewData) {
                state.uiState.processingHandedness = Handedness_t::Left;
            }
            DrawGlove("Left Glove", "glove_left", state.gloveLeft, state);
            if (state.uiState.page == ScreenState_t::ScreenStateViewData) {
                state.uiState.processingHandedness = Handedness_t::Right;
            }
            DrawGlove("Right Glove", "glove_right", state.gloveRight, state);

            // @TODO: Break settings into function / tab
            ImGui::Spacing();
            ImGui::Checkbox("Automatically launch with SteamVR", &state.doAutoLaunch);
            break;
        }
        case ScreenState_t::ScreenStateCalibrateJoystick: {
            DrawCalibrateJoystick(state);
            break;
        }
        case ScreenState_t::ScreenStateCalibrateFingers: {
            DrawCalibrateFingers(state);
            break;
        }
        case ScreenState_t::ScreenStateCalibrateOffset: {
            DrawCalibrateOffsets(state);
            break;
        }
    }

    ImGui::End();
}