#define NOMINMAX
#include "overlay_app.hpp"
#include "contact_glove/serial_communication.hpp"
#include "ipc_client.hpp"
#include "app_state.hpp"
#include "configuration.hpp"

void ForwardDataToDriver(AppState& state, IPCClient& ipcClient);
void ProcessGlove(protocol::ContactGloveState& glove, std::chrono::steady_clock::time_point gloveConnected);
void UpdateGloveInputState(AppState& state);

// Tell the GPU drivers to give the overlay priority over other apps, it's a driver after all
extern "C" __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
extern "C" __declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001;

// 2 second timeout for the gloves
constexpr auto GLOVE_TIMEOUT = std::chrono::steady_clock::time_point::duration(std::chrono::milliseconds(2000));

// Props for the overlay
#define OPENVR_APPLICATION_KEY "hyblocker.DriverFreeScuba"
static vr::VROverlayHandle_t s_overlayMainHandle;
static vr::VROverlayHandle_t s_overlayThumbnailHandle;

void ActivateMultipleDrivers()
{
    vr::EVRSettingsError vrSettingsError;
    bool enabled = vr::VRSettings()->GetBool(vr::k_pch_SteamVR_Section, vr::k_pch_SteamVR_ActivateMultipleDrivers_Bool, &vrSettingsError);

    if (vrSettingsError != vr::VRSettingsError_None)
    {
        std::string err = "Could not read \"" + std::string(vr::k_pch_SteamVR_ActivateMultipleDrivers_Bool) + "\" setting: "
            + vr::VRSettings()->GetSettingsErrorNameFromEnum(vrSettingsError);

        throw std::runtime_error(err);
    }

    if (!enabled)
    {
        vr::VRSettings()->SetBool(vr::k_pch_SteamVR_Section, vr::k_pch_SteamVR_ActivateMultipleDrivers_Bool, true, &vrSettingsError);
        if (vrSettingsError != vr::VRSettingsError_None)
        {
            std::string err = "Could not set \"" + std::string(vr::k_pch_SteamVR_ActivateMultipleDrivers_Bool) + "\" setting: "
                + vr::VRSettings()->GetSettingsErrorNameFromEnum(vrSettingsError);

            throw std::runtime_error(err);
        }

        std::cerr << "Enabled \"" << vr::k_pch_SteamVR_ActivateMultipleDrivers_Bool << "\" setting" << std::endl;
    }
    else
    {
        std::cerr << "\"" << vr::k_pch_SteamVR_ActivateMultipleDrivers_Bool << "\" setting previously enabled" << std::endl;
    }
}

void InitVR()
{
    auto initError = vr::VRInitError_None;
    vr::VR_Init(&initError, vr::VRApplication_Other);
    if (initError != vr::VRInitError_None)
    {
        auto error = vr::VR_GetVRInitErrorAsEnglishDescription(initError);
        throw std::runtime_error("OpenVR error:" + std::string(error));
    }

    if (!vr::VR_IsInterfaceVersionValid(vr::IVRSystem_Version))
    {
        throw std::runtime_error("OpenVR error: Outdated IVRSystem_Version");
    }
    else if (!vr::VR_IsInterfaceVersionValid(vr::IVRSettings_Version))
    {
        throw std::runtime_error("OpenVR error: Outdated IVRSettings_Version");
    }
    else if (!vr::VR_IsInterfaceVersionValid(vr::IVROverlay_Version))
    {
        throw std::runtime_error("OpenVR error: Outdated IVROverlay_Version");
    }

    ActivateMultipleDrivers();
}

void TryCreateVrOverlay() {
    if (s_overlayMainHandle || !vr::VROverlay()) {
        return;
    }

    vr::VROverlayError err = vr::VROverlay()->CreateDashboardOverlay(
        OPENVR_APPLICATION_KEY, "FreeScuba",
        &s_overlayMainHandle, &s_overlayThumbnailHandle
    );

    if (err == vr::VROverlayError_KeyInUse) {
        throw std::runtime_error("Another instance of FreeScuba is already running!");
    } else if (err != vr::VROverlayError_None) {
        throw std::runtime_error("Failed to create SteamVR Overlay with error " + std::string(vr::VROverlay()->GetOverlayErrorNameFromEnum(err)));
    }

    vr::VROverlay()->SetOverlayWidthInMeters(s_overlayMainHandle, 3.0f);
    vr::VROverlay()->SetOverlayInputMethod(s_overlayMainHandle, vr::VROverlayInputMethod_Mouse);
    vr::VROverlay()->SetOverlayFlag(s_overlayMainHandle, vr::VROverlayFlags_SendVRDiscreteScrollEvents, true);

    // @TODO: Icon
}

int main() {

    try {

        F_CRC_InitialiseTable();

        // Global serial manager
        static SerialCommunicationManager man = {};
        static AppState state = {};

        LoadConfiguration(state);

        // Glove timing
        static std::chrono::steady_clock::time_point gloveLeftConnected  = std::chrono::steady_clock::time_point::min();
        static std::chrono::steady_clock::time_point gloveRightConnected = std::chrono::steady_clock::time_point::min();
        // static std::chrono::steady_clock::time_point gloveLeftConnected  = std::chrono::system_clock::from_time_t(0);
        // static std::chrono::steady_clock::time_point gloveRightConnected = std::chrono::system_clock::from_time_t(0);

        // Init SteamVR
        InitVR();

        // Setup IPC
        IPCClient ipcClient;
        ipcClient.Connect();
        state.ipcClient = &ipcClient;

        // Serial data listener
        man.BeginListener(
            [&](const ContactGloveDevice handedness, const GloveInputData_t& inputData) {
                // gloveLeftConnected = std::chrono::high_resolution_clock::now();
                // gloveRightConnected = std::chrono::high_resolution_clock::now();

                switch (handedness) {
                    case ContactGloveDevice::LeftGlove:
                        state.gloveLeft.hasMagnetra         = inputData.hasMagnetra;
                        state.gloveLeft.systemUp            = inputData.systemUp;
                        state.gloveLeft.systemDown          = inputData.systemDown;
                        state.gloveLeft.buttonUp            = inputData.buttonUp;
                        state.gloveLeft.buttonDown          = inputData.buttonDown;
                        state.gloveLeft.joystickClick       = inputData.joystickClick;
                        state.gloveLeft.joystickXRaw        = inputData.joystickX;
                        state.gloveLeft.joystickYRaw        = inputData.joystickY;
                        break;
                    case ContactGloveDevice::RightGlove:
                        state.gloveRight.hasMagnetra        = inputData.hasMagnetra;
                        state.gloveRight.systemUp           = inputData.systemUp;
                        state.gloveRight.systemDown         = inputData.systemDown;
                        state.gloveRight.buttonUp           = inputData.buttonUp;
                        state.gloveRight.buttonDown         = inputData.buttonDown;
                        state.gloveRight.joystickClick      = inputData.joystickClick;
                        state.gloveRight.joystickXRaw       = inputData.joystickX;
                        state.gloveRight.joystickYRaw       = inputData.joystickY;
                        break;
                }
            },

            [&](const ContactGloveDevice handedness, const GlovePacketFingers_t& fingerData) {
                switch (handedness) {
                    case ContactGloveDevice::LeftGlove:
                        gloveLeftConnected = std::chrono::high_resolution_clock::now();
                        state.gloveLeft.isConnected         = true;
                        state.gloveLeft.thumbRootRaw        = fingerData.fingerThumbRoot;
                        state.gloveLeft.thumbTipRaw         = fingerData.fingerThumbTip;
                        state.gloveLeft.indexRootRaw        = fingerData.fingerIndexRoot;
                        state.gloveLeft.indexTipRaw         = fingerData.fingerIndexTip;
                        state.gloveLeft.middleRootRaw       = fingerData.fingerMiddleRoot;
                        state.gloveLeft.middleTipRaw        = fingerData.fingerMiddleTip;
                        state.gloveLeft.ringRootRaw         = fingerData.fingerRingRoot;
                        state.gloveLeft.ringTipRaw          = fingerData.fingerRingTip;
                        state.gloveLeft.pinkyRootRaw        = fingerData.fingerPinkyRoot;
                        state.gloveLeft.pinkyTipRaw         = fingerData.fingerPinkyTip;
                        break;
                    case ContactGloveDevice::RightGlove:
                        gloveRightConnected = std::chrono::high_resolution_clock::now();
                        state.gloveRight.isConnected        = true;
                        state.gloveRight.thumbRootRaw       = fingerData.fingerThumbRoot;
                        state.gloveRight.thumbTipRaw        = fingerData.fingerThumbTip;
                        state.gloveRight.indexRootRaw       = fingerData.fingerIndexRoot;
                        state.gloveRight.indexTipRaw        = fingerData.fingerIndexTip;
                        state.gloveRight.middleRootRaw      = fingerData.fingerMiddleRoot;
                        state.gloveRight.middleTipRaw       = fingerData.fingerMiddleTip;
                        state.gloveRight.ringRootRaw        = fingerData.fingerRingRoot;
                        state.gloveRight.ringTipRaw         = fingerData.fingerRingTip;
                        state.gloveRight.pinkyRootRaw       = fingerData.fingerPinkyRoot;
                        state.gloveRight.pinkyTipRaw        = fingerData.fingerPinkyTip;
                        break;
                }
            },

            [&](const DevicesStatus_t& status) {
                // Only update the timeout if the battery is valid
                if (status.gloveLeftBattery != CONTACT_GLOVE_INVALID_BATTERY) {
                    gloveLeftConnected = std::chrono::high_resolution_clock::now();
                }
                if (status.gloveRightBattery != CONTACT_GLOVE_INVALID_BATTERY) {
                    gloveRightConnected = std::chrono::high_resolution_clock::now();
                }

                state.gloveLeft.gloveBatteryRaw             = status.gloveLeftBattery;
                state.gloveRight.gloveBatteryRaw            = status.gloveRightBattery;
            },

            [&](const DevicesFirmware_t& firmware) {
                state.gloveLeft.firmwareMajor               = firmware.gloveLeftMajor;
                state.gloveLeft.firmwareMinor               = firmware.gloveLeftMinor;
                state.gloveRight.firmwareMajor              = firmware.gloveRightMajor;
                state.gloveRight.firmwareMinor              = firmware.gloveRightMinor;
            }
        );

        if (FreeScuba::Overlay::StartWindow()) {
            bool doExecute = true;
            while (doExecute) {
                TryCreateVrOverlay();

                state.dongleAvailable = man.IsConnected();
                ProcessGlove(state.gloveLeft, gloveLeftConnected);
                ProcessGlove(state.gloveRight, gloveRightConnected);
                UpdateGloveInputState(state);

                doExecute = FreeScuba::Overlay::UpdateNativeWindow(state, s_overlayMainHandle);
                
                if (doExecute) {
                    ForwardDataToDriver(state, ipcClient);
                }
            }

            SaveConfiguration(state);
            FreeScuba::Overlay::DestroyWindow();
        }

        state.ipcClient = nullptr;
        man.Disconnect();
    }
    catch (std::runtime_error& e)
    {
        std::cerr << "Runtime error: " << e.what() << std::endl;
        wchar_t message[1024];
        swprintf(message, 1024, L"%hs", e.what());
        MessageBoxW(nullptr, message, L"Runtime Error", 0);
    }
}

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define CLAMP(t,a,b) (MAX(MIN(t, b), a))

void ProcessGlove(protocol::ContactGloveState& glove, std::chrono::steady_clock::time_point gloveConnected) {

    // Compute whether we should consider the glove as connected or not
    auto delta = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - gloveConnected);
    glove.isConnected = delta < GLOVE_TIMEOUT && gloveConnected != std::chrono::steady_clock::time_point::min();

    // Only process the rest of the data IF and only IF the glove is connected
    if (glove.isConnected) {
        // Only process magnetra stuff if magnetra is connected
        if (glove.hasMagnetra) {
            float joystickX = 2.0f * (glove.joystickXRaw - glove.calibration.joystick.XMin) / (float)MAX(glove.calibration.joystick.XMax - glove.calibration.joystick.XMin, 0.0f) - 1.0f;
            float joystickY = 2.0f * (glove.joystickYRaw - glove.calibration.joystick.YMin) / (float)MAX(glove.calibration.joystick.YMax - glove.calibration.joystick.YMin, 0.0f) - 1.0f;

            // Normalize the axis if out of range
            if (joystickX * joystickX + joystickY * joystickY > 1.0f) {
                double length = sqrt(joystickX * joystickX + joystickY * joystickY);
                joystickX = (float) (joystickX / length);
                joystickY = (float) (joystickY / length);
            }

            // Re-orient the up forward vector based on user calibration
            // Use a negated 2D rotation matrix
            float orientedJoystickX = joystickX * cos(glove.calibration.joystick.forwardAngle) + joystickY * -sin(glove.calibration.joystick.forwardAngle);
            float orientedJoystickY = joystickX * sin(glove.calibration.joystick.forwardAngle) + joystickY * cos(glove.calibration.joystick.forwardAngle);

            glove.joystickX = orientedJoystickX;
            glove.joystickY = orientedJoystickY;
            glove.joystickXUnfiltered = orientedJoystickX;
            glove.joystickYUnfiltered = orientedJoystickY;

            // Apply deadzone
            // Compare vector magnitudes, if smaller than threshold 0 out
            if (glove.joystickX * glove.joystickX + glove.joystickY * glove.joystickY < glove.calibration.joystick.threshold) {
                glove.joystickX = 0.0f;
                glove.joystickY = 0.0f;
            }
        }
        else {
            // Default values, i.e. no joystick / buttons
            glove.joystickX     = 0.0f;
            glove.joystickY     = 0.0f;
            glove.buttonDown    = false;
            glove.buttonUp      = false;
            glove.systemDown    = false;
            glove.systemUp      = false;
            glove.joystickClick = false;
        }

        // Battery should only update if it's not invalid (sometimes the battery status is invalid)
        if (glove.gloveBatteryRaw != CONTACT_GLOVE_INVALID_BATTERY) {
            uint8_t gloveBatteryClamped = CLAMP(glove.gloveBatteryRaw, 0, 100);
            uint32_t gloveBatteryDelta = glove.gloveBattery - gloveBatteryClamped;

            // @FIXME: Battery level can get stuck at low percentages for some reason. Need a better filter to eliminate erroneous values
            //         Probably because on rare occasions it may send a low battery value (??)

            // If the change in battery level is < 5%, accept it, any higher is invalid
            if (glove.gloveBattery == CONTACT_GLOVE_INVALID_BATTERY || std::abs(gloveBatteryDelta < GLOVE_BATTERY_THRESHOLD)) {
                glove.gloveBattery = gloveBatteryClamped;
            }
        }

        // Apply finger calibration to raw finger data

        // Helper macro because 80% of the code is copy paste par joint names
        // Remaps such that rest is 0.0, and close is +1.0, and prevents values > 1.0 being output
#define APPLY_FINGER_CALIBRATION(joint) \
        glove.joint = std::min((glove.joint##Raw - glove.calibration.fingers.joint.rest) / (float) (glove.calibration.fingers.joint.close - glove.calibration.fingers.joint.rest), 1.0f)

        APPLY_FINGER_CALIBRATION(thumbRoot);
        APPLY_FINGER_CALIBRATION(thumbTip);
        APPLY_FINGER_CALIBRATION(indexRoot);
        APPLY_FINGER_CALIBRATION(indexTip);
        APPLY_FINGER_CALIBRATION(middleRoot);
        APPLY_FINGER_CALIBRATION(middleTip);
        APPLY_FINGER_CALIBRATION(ringRoot);
        APPLY_FINGER_CALIBRATION(ringTip);
        APPLY_FINGER_CALIBRATION(pinkyRoot);
        APPLY_FINGER_CALIBRATION(pinkyTip);

#undef APPLY_FINGER_CALIBRATION

    } else {
        glove.gloveBattery = CONTACT_GLOVE_INVALID_BATTERY;
        glove.joystickX = 0.0f;
        glove.joystickY = 0.0f;
    }
}

// Process input for the UI to be able to handle inputs properly
void UpdateGloveInputState(AppState& state) {
    // Left previous frame
    state.uiState.gloveButtons.prevLeft.buttonDown          = state.uiState.gloveButtons.left.buttonDown;
    state.uiState.gloveButtons.prevLeft.buttonUp            = state.uiState.gloveButtons.left.buttonUp;
    state.uiState.gloveButtons.prevLeft.systemDown          = state.uiState.gloveButtons.left.systemDown;
    state.uiState.gloveButtons.prevLeft.systemUp            = state.uiState.gloveButtons.left.systemUp;
    state.uiState.gloveButtons.prevLeft.joystickClick       = state.uiState.gloveButtons.left.joystickClick;

    // Right previous frame
    state.uiState.gloveButtons.prevRight.buttonDown         = state.uiState.gloveButtons.right.buttonDown;
    state.uiState.gloveButtons.prevRight.buttonUp           = state.uiState.gloveButtons.right.buttonUp;
    state.uiState.gloveButtons.prevRight.systemDown         = state.uiState.gloveButtons.right.systemDown;
    state.uiState.gloveButtons.prevRight.systemUp           = state.uiState.gloveButtons.right.systemUp;
    state.uiState.gloveButtons.prevRight.joystickClick      = state.uiState.gloveButtons.right.joystickClick;

    // Left current frame
    state.uiState.gloveButtons.left.buttonDown              = state.gloveLeft.buttonDown;
    state.uiState.gloveButtons.left.buttonUp                = state.gloveLeft.buttonUp;
    state.uiState.gloveButtons.left.systemDown              = state.gloveLeft.systemDown;
    state.uiState.gloveButtons.left.systemUp                = state.gloveLeft.systemUp;
    state.uiState.gloveButtons.left.joystickClick           = state.gloveLeft.joystickClick;

    // Right current frame
    state.uiState.gloveButtons.right.buttonDown             = state.gloveRight.buttonDown;
    state.uiState.gloveButtons.right.buttonUp               = state.gloveRight.buttonUp;
    state.uiState.gloveButtons.right.systemDown             = state.gloveRight.systemDown;
    state.uiState.gloveButtons.right.systemUp               = state.gloveRight.systemUp;
    state.uiState.gloveButtons.right.joystickClick          = state.gloveRight.joystickClick;

    // Left released
    state.uiState.gloveButtons.releasedLeft.buttonDown      = state.uiState.gloveButtons.prevLeft.buttonDown      == true && state.uiState.gloveButtons.prevLeft.buttonDown      != state.uiState.gloveButtons.left.buttonDown;
    state.uiState.gloveButtons.releasedLeft.buttonUp        = state.uiState.gloveButtons.prevLeft.buttonUp        == true && state.uiState.gloveButtons.prevLeft.buttonUp        != state.uiState.gloveButtons.left.buttonUp;
    state.uiState.gloveButtons.releasedLeft.systemDown      = state.uiState.gloveButtons.prevLeft.systemDown      == true && state.uiState.gloveButtons.prevLeft.systemDown      != state.uiState.gloveButtons.left.systemDown;
    state.uiState.gloveButtons.releasedLeft.systemUp        = state.uiState.gloveButtons.prevLeft.systemUp        == true && state.uiState.gloveButtons.prevLeft.systemUp        != state.uiState.gloveButtons.left.systemUp;
    state.uiState.gloveButtons.releasedLeft.joystickClick   = state.uiState.gloveButtons.prevLeft.joystickClick   == true && state.uiState.gloveButtons.prevLeft.joystickClick   != state.uiState.gloveButtons.left.joystickClick;

    // Right released
    state.uiState.gloveButtons.releasedRight.buttonDown     = state.uiState.gloveButtons.prevRight.buttonDown     == true && state.uiState.gloveButtons.prevRight.buttonDown     != state.uiState.gloveButtons.right.buttonDown;
    state.uiState.gloveButtons.releasedRight.buttonUp       = state.uiState.gloveButtons.prevRight.buttonUp       == true && state.uiState.gloveButtons.prevRight.buttonUp       != state.uiState.gloveButtons.right.buttonUp;
    state.uiState.gloveButtons.releasedRight.systemDown     = state.uiState.gloveButtons.prevRight.systemDown     == true && state.uiState.gloveButtons.prevRight.systemDown     != state.uiState.gloveButtons.right.systemDown;
    state.uiState.gloveButtons.releasedRight.systemUp       = state.uiState.gloveButtons.prevRight.systemUp       == true && state.uiState.gloveButtons.prevRight.systemUp       != state.uiState.gloveButtons.right.systemUp;
    state.uiState.gloveButtons.releasedRight.joystickClick  = state.uiState.gloveButtons.prevRight.joystickClick  == true && state.uiState.gloveButtons.prevRight.joystickClick  != state.uiState.gloveButtons.right.joystickClick;
}

static char deviceRole[vr::k_unMaxPropertyStringSize];

void ForwardDataToDriver(AppState& state, IPCClient& ipcClient) {

    protocol::Request req = {};

    uint32_t trackerIdLeft  = CONTACT_GLOVE_INVALID_DEVICE_ID;
    uint32_t trackerIdRight = CONTACT_GLOVE_INVALID_DEVICE_ID;

    // Search for the handed tracker, only if either of the gloves are connected
    if (state.gloveLeft.isConnected == true || state.gloveRight.isConnected == true) {
        // Skip 0 as it's reserved for the HMD
        for (int i = 1; i < vr::k_unMaxTrackedDeviceCount; i++) {
            // Only look at connected devices
            if (vr::VRSystem()->IsTrackedDeviceConnected(i)) {

                // Filter by trackers
                uint32_t deviceClass = vr::VRSystem()->GetInt32TrackedDeviceProperty(i, vr::Prop_DeviceClass_Int32);
                if (deviceClass == vr::TrackedDeviceClass_GenericTracker) {

                    // Get the tracker role
                    memset(deviceRole, 0, sizeof(deviceRole));
                    vr::VRSystem()->GetStringTrackedDeviceProperty(i, vr::Prop_ControllerType_String, deviceRole, sizeof deviceRole);
                    vr::ETrackedControllerRole controllerHand = static_cast<vr::ETrackedControllerRole>(vr::VRSystem()->GetInt32TrackedDeviceProperty(i, vr::Prop_ControllerRoleHint_Int32));

                    // Handed role
                    if (strcmp(deviceRole, "vive_tracker_handed") == 0 || // Vive Tracker, Tundra Tracker
                        strcmp(deviceRole, "lighthouse_tracker")  == 0 || // Manus Tracker
                        strcmp(deviceRole, "etee_tracker_handed") == 0    // Etee Tracker
                        ) {

                        if (controllerHand == vr::TrackedControllerRole_RightHand) {
                            // Found right tracker
                            if (trackerIdRight == CONTACT_GLOVE_INVALID_DEVICE_ID) {
                                trackerIdRight = i;
                            }
                            if (trackerIdLeft != CONTACT_GLOVE_INVALID_DEVICE_ID) {
                                break;
                            }
                        } else if (controllerHand == vr::TrackedControllerRole_LeftHand) {
                            // Found left tracker
                            if (trackerIdLeft == CONTACT_GLOVE_INVALID_DEVICE_ID) {
                                trackerIdLeft = i;
                            }
                            if (trackerIdRight != CONTACT_GLOVE_INVALID_DEVICE_ID) {
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    if (state.gloveLeft.isConnected == true) {
        state.gloveLeft.trackerIndex = trackerIdLeft;
        req.type = protocol::RequestType::RequestUpdateGloveLeftState;
        memcpy(&req.gloveData, &state.gloveLeft, sizeof(protocol::ContactGloveState));
        // Adjust angle
        ipcClient.SendBlocking(req);
    } else {
        req.type = protocol::RequestType::RequestUpdateGloveLeftState;
        req.gloveData.isConnected = false;
        ipcClient.SendBlocking(req);
    }

    if (state.gloveRight.isConnected == true) {
        state.gloveRight.trackerIndex = trackerIdRight;
        req.type = protocol::RequestType::RequestUpdateGloveRightState;
        memcpy(&req.gloveData, &state.gloveRight, sizeof(protocol::ContactGloveState));
        // Adjust angle
        ipcClient.SendBlocking(req);
    } else {
        req.type = protocol::RequestType::RequestUpdateGloveRightState;
        req.gloveData.isConnected = false;
        ipcClient.SendBlocking(req);
    }
}