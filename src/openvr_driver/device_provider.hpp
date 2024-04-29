#pragma once

#include <mutex>

#include "openvr_driver.h"
#include "driverlog.hpp"
#include "contactglove_device.hpp"
#include "ipc_server.hpp"

class DeviceProvider : public vr::IServerTrackedDeviceProvider {
public:
    // Inherited via IServerTrackedDeviceProvider
    vr::EVRInitError Init(vr::IVRDriverContext* pDriverContext) override;
    void Cleanup() override;
    const char* const* GetInterfaceVersions() override;
    void RunFrame() override;
    bool ShouldBlockStandbyMode() override;
    void EnterStandby() override;
    void LeaveStandby() override;

public:
    DeviceProvider() : m_server(this), m_poseMutex(false) { memset(m_poseCache, 0, sizeof m_poseCache); }

    bool HandleDevicePoseUpdated(uint32_t openVRID, vr::DriverPose_t& pose);

    void HandleGloveUpdate(protocol::ContactGloveState_t updateState, bool isLeft);

    vr::DriverPose_t GetCachedPose(uint32_t trackedDeviceIndex);

private:
    Hekky::IPC::IPCServer m_server;

    std::atomic_bool m_poseMutex;
    vr::DriverPose_t m_poseCache[vr::k_unMaxTrackedDeviceCount];

    ContactGloveDevice m_leftGlove{ this, true };
    ContactGloveDevice m_rightGlove{ this, false };
};