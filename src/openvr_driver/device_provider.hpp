#pragma once

#include "openvr_driver.h"
#include "driverlog.hpp"
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
    DeviceProvider() : m_server(this) {}

    void HandleFingersUpdate(protocol::FingersUpdate fingersUpdate);
    void HandleInputUpdate(protocol::InputUpdate inputUpdate);
    void HandleGloveStateUpdate(protocol::StateUpdate updateState);

private:
    IPCServer m_server;
};