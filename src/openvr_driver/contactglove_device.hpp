#pragma once

#include <mutex>
#include <thread>

#include "openvr_driver.h"
#include "../ipc_protocol.hpp"
#include "driverlog.hpp"
#include "hand_simulation.hpp"

class DeviceProvider;
const double INPUT_FREQUENCY = 1000.0 / 90.0; // 90Hz input thread

const short NUM_BONES = static_cast<short>(HandSkeletonBone::kHandSkeletonBone_Count);

enum class KnuckleDeviceComponentIndex_t : uint8_t {
    SystemClick = 0,
    SystemTouch,
    TriggerClick,
    TriggerValue,
    TrackpadX,
    TrackpadY,
    TrackpadTouch,
    TrackpadForce,
    GripTouch,
    GripForce,
    GripValue,
    ThumbstickClick,
    ThumbstickTouch,
    ThumbstickX,
    ThumbstickY,
    AClick,
    ATouch,
    BClick,
    BTouch,
    FingerThumb,
    FingerIndex,
    FingerMiddle,
    FingerRing,
    FingerPinky,
    _Count
};

// SteamVR tracked device
class ContactGloveDevice : public vr::ITrackedDeviceServerDriver {

private:
    struct ThresholdState {
    public:
        float value;
        bool isActive;
    };

public:
    ContactGloveDevice(DeviceProvider* devProvider, bool isLeft);

    vr::EVRInitError Activate(uint32_t unObjectId) override;
    void Deactivate() override;
    void EnterStandby() override;
    void* GetComponent(const char* pchComponentNameAndVersion) override;
    void DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize) override;
    vr::DriverPose_t GetPose() override;

public:
    // Steamvr Tick
    void Tick();
    void Update(const protocol::ContactGloveState_t& updateState);
    void UpdateSkeletalInput(const protocol::ContactGloveState_t& updateState);
    void UpdateInputs(const protocol::ContactGloveState_t& updateState);
    void SetupProps();
    void ApproximateCurls(const protocol::ContactGloveState_t& updateState);
    float ApproximateSingleFingerCurl(HandSkeletonBone metacarpal, HandSkeletonBone distal);

    void PoseUpdateThread();
    void InputUpdateThread();

private:
    void HandleGesture(ThresholdState& param, const protocol::ContactGloveState_t::CalibrationData_t::GestureThreshold_t& thresholds, const float value);

private:
    uint32_t m_deviceId;
    uint32_t m_shadowDevice; // The tracker we shall be stealing a pose from
    bool m_isLeft;
    bool m_isActiveInSteamVR;
    bool m_isConnectedMainThreadLocal;
    bool m_ignorePosesThreadLocal;
    std::atomic_bool m_isConnected;

    DeviceProvider* m_devProvider;

    vr::DriverPose_t m_lastPose;

    std::string m_serial;
    std::string m_deviceManufacturer;

    std::atomic_bool m_poseUpdateExecute;
    std::atomic_bool m_ignorePoses;
    std::thread m_poseUpdateThread;
    std::atomic_bool m_doInput;
    std::atomic_bool m_inputUpdateExecute;
    std::thread m_inputUpdateThread;

    vr::PropertyContainerHandle_t m_ulProps;

    // Approximated curl values (from skeletal input processing)
    float m_curlThumb;
    float m_curlIndex;
    float m_curlMiddle;
    float m_curlRing;
    float m_curlPinky;

    // Threshold values for inputs
    ThresholdState m_thumbActivation;
    ThresholdState m_triggerActivation;
    ThresholdState m_gripActivation;

    vr::VRInputComponentHandle_t m_hapticInputHandle;
    vr::VRInputComponentHandle_t m_skeletalComponentHandle;
    vr::VRBoneTransform_t m_handTransforms[NUM_BONES];
    vr::VRInputComponentHandle_t m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::_Count)];

    protocol::ContactGloveState_t::CalibrationData_t::PoseOffset_t m_poseOffset;
    protocol::ContactGloveState_t m_lastState;

    // Skeletal input simulation
    GloveHandSimulation m_handSimulation;
};