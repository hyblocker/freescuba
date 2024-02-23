#include "device_provider.hpp"

vr::EVRInitError DeviceProvider::Init(vr::IVRDriverContext* pDriverContext) {
    VR_INIT_SERVER_DRIVER_CONTEXT(pDriverContext);

    vr::VRDriverLog()->Log("FreeScuba::DeviceProvider::Init()");

    // @TODO: Inject minhook here
    m_server.Run();

    return vr::VRInitError_None;
}

void DeviceProvider::Cleanup() {
    vr::VRDriverLog()->Log("ServerTrackedDeviceProvider::Cleanup()");
    m_server.Stop();
    // @TODO: Uninject minhook here
    VR_CLEANUP_SERVER_DRIVER_CONTEXT();
}

const char* const* DeviceProvider::GetInterfaceVersions() {
    return vr::k_InterfaceVersions;
}

void DeviceProvider::RunFrame() {

}

bool DeviceProvider::ShouldBlockStandbyMode() {
    return false;
}

void DeviceProvider::EnterStandby() {

}

void DeviceProvider::LeaveStandby() {

}