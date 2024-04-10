#pragma once

#include "contact_glove/serial_communication.hpp"
#include "ipc_client.hpp"
#include <openvr.h>

enum class ScreenState {
    ScreenStateViewData,
    ScreenStateCalibrateJoystick,
    ScreenStateCalibrateFingers,
    ScreenStateCalibrateOffset,
};

enum class Handedness {
    Left,
    Right
};

enum class CalibrationState {
    State_Entering = 0,
    // Stages for joystick calibration
    Joystick_DiscoverBounds = 1,
    Joystick_DiscoverNeutral,
    Joystick_DiscoverForward,

    // Stages for finger tracking calibration
    Fingers_DiscoverNeutral = 1,
    Fingers_DiscoverClosed,
    Fingers_DiscoverBackwardsBend,

    // Stages for pose offset calibration
    PoseOffset_FindInitialPose = 1,
    PoseOffset_Moving,
    PoseOffset_Set,
};

struct AppState {
public:
    AppState();

    // Protocol state
    protocol::ContactGloveState gloveLeft;
    protocol::ContactGloveState gloveRight;
    bool dongleAvailable;

    IPCClient* ipcClient;

    // Ui State
    struct UiState {
        ScreenState page;
        CalibrationState calibrationState;
        Handedness processingHandedness;

        // Buffer for the previous calibration state
        protocol::ContactGloveState::CalibrationData oldCalibration;
        // Buffer for the temporary, current calibration state. This is not forwarded to the SteamVR driver until the calibration is complete.
        protocol::ContactGloveState::CalibrationData currentCalibration;

        // For joystick calibration
        uint16_t joystickForwardX;
        uint16_t joystickForwardY;

        struct {
            struct UiGloveButtonsState {
                bool systemUp;
                bool systemDown;
                bool buttonUp;
                bool buttonDown;
                bool joystickClick;
            };

            UiGloveButtonsState left;
            UiGloveButtonsState right;
            UiGloveButtonsState releasedLeft;
            UiGloveButtonsState releasedRight;
            UiGloveButtonsState prevLeft;
            UiGloveButtonsState prevRight;

        } gloveButtons;

        vr::HmdVector3d_t initialTrackerPos;
        vr::HmdQuaternion_t initialTrackerRot;
    } uiState;
};