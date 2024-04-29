#pragma once

#include "contact_glove/serial_communication.hpp"
#include "ipc_client.hpp"
#include "ring_buffer.hpp"
#include <openvr.h>

// #define BATTERY_WINDOW_SIZE 128
constexpr uint8_t BATTERY_WINDOW_SIZE = 128;

enum class ScreenState_t {
    ScreenStateViewData,
    ScreenStateCalibrateJoystick,
    ScreenStateCalibrateFingers,
    ScreenStateCalibrateOffset,
};

enum class Handedness_t {
    Left,
    Right
};

enum class CalibrationState_t {
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
    protocol::ContactGloveState_t gloveLeft;
    protocol::ContactGloveState_t gloveRight;
    bool dongleAvailable;

    IPCClient* ipcClient;

    // For the SteamVR Overlay
    bool doAutoLaunch;

    // Ui State
    struct UiState_t {

        ScreenState_t page;
        CalibrationState_t calibrationState;
        Handedness_t processingHandedness;

        // Buffer for the previous calibration state
        protocol::ContactGloveState_t::CalibrationData_t oldCalibration;
        // Buffer for the temporary, current calibration state. This is not forwarded to the SteamVR driver until the calibration is complete.
        protocol::ContactGloveState_t::CalibrationData_t currentCalibration;

        MostCommonElementRingBuffer leftGloveBatteryBuffer;
        MostCommonElementRingBuffer rightGloveBatteryBuffer;

        // For joystick calibration
        uint16_t joystickForwardX;
        uint16_t joystickForwardY;

        struct {
            struct UiGloveButtonsState_t {
                bool systemUp;
                bool systemDown;
                bool buttonUp;
                bool buttonDown;
                bool joystickClick;
            };

            UiGloveButtonsState_t left;
            UiGloveButtonsState_t right;
            UiGloveButtonsState_t releasedLeft;
            UiGloveButtonsState_t releasedRight;
            UiGloveButtonsState_t prevLeft;
            UiGloveButtonsState_t prevRight;

        } gloveButtons;

        vr::HmdVector3d_t initialTrackerPos;
        vr::HmdQuaternion_t initialTrackerRot;
    } uiState;
};