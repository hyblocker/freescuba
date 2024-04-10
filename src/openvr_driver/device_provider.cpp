#include "device_provider.hpp"
#include "interface_hook_injector.hpp"

vr::EVRInitError DeviceProvider::Init(vr::IVRDriverContext* pDriverContext) {
    VR_INIT_SERVER_DRIVER_CONTEXT(pDriverContext);

    LOG("FreeScuba::DeviceProvider::Init()");

    InjectHooks(this, pDriverContext);
    m_server.Run();

    return vr::VRInitError_None;
}

void DeviceProvider::Cleanup() {
    LOG("ServerTrackedDeviceProvider::Cleanup()");
    m_server.Stop();
    DisableHooks();
    VR_CLEANUP_SERVER_DRIVER_CONTEXT();
}

const char* const* DeviceProvider::GetInterfaceVersions() {
    return vr::k_InterfaceVersions;
}

void DeviceProvider::RunFrame() {
    m_leftGlove.Tick();
    m_rightGlove.Tick();
}

bool DeviceProvider::ShouldBlockStandbyMode() {
    return false;
}

void DeviceProvider::EnterStandby() {

}

void DeviceProvider::LeaveStandby() {

}

void DeviceProvider::HandleGloveUpdate(protocol::ContactGloveState updateState, bool isLeft) {
    if (isLeft) {
        m_leftGlove.Update(updateState);
    } else {
        m_rightGlove.Update(updateState);
    }
}

bool DeviceProvider::HandleDevicePoseUpdated(uint32_t openVRID, vr::DriverPose_t& pose) {
    m_poseMutex.exchange(true);
    m_poseCache[openVRID] = pose;
    m_poseMutex.exchange(false);

    return true;
}

vr::DriverPose_t DeviceProvider::GetCachedPose(uint32_t trackedDeviceIndex) {
    m_poseMutex.exchange(true);
    vr::DriverPose_t pose = m_poseCache[trackedDeviceIndex];
    m_poseMutex.exchange(false);
    return pose;
}