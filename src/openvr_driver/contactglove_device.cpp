#include "contactglove_device.hpp"
#include "device_provider.hpp"
#include "maths.hpp"

ContactGloveDevice::ContactGloveDevice(DeviceProvider* devProvider, bool isLeft)
    :	m_isLeft(isLeft),
        m_devProvider(devProvider),
        m_isActiveInSteamVR(false),
        m_isConnectedMainThreadLocal(false),
        m_isConnected(false),
        m_poseUpdateExecute(false),
        m_ignorePoses(false),
        m_ignorePosesThreadLocal(false),
        m_doInput(false),
        m_inputUpdateExecute(false),
        m_thumbActivation({}),
        m_triggerActivation({}),
        m_gripActivation({}),
        m_poseOffset({}),
        m_lastState({}),
        m_curlThumb(0),
        m_curlIndex(0),
        m_curlMiddle(0),
        m_curlRing(0),
        m_curlPinky(0),
        m_lastPose(vr::DriverPose_t()),
        m_shadowDevice(CONTACT_GLOVE_INVALID_DEVICE_ID),
        m_handSimulation(),
        m_ulProps(vr::k_ulInvalidPropertyContainer),
        m_hapticInputHandle(vr::k_ulInvalidInputComponentHandle),
        m_skeletalComponentHandle(vr::k_ulInvalidInputComponentHandle),
        m_deviceId(vr::k_unTrackedDeviceIndexInvalid) {

    if (isLeft) {
        m_serial = "ContactGlove-Left";
    } else {
        m_serial = "ContactGlove-Right";
    }
    m_deviceManufacturer = "Diver-X";
    memset(m_inputComponentHandles, vr::k_ulInvalidInputComponentHandle, sizeof m_inputComponentHandles);
    memset(m_handTransforms, 0, sizeof m_inputComponentHandles);
}

vr::EVRInitError ContactGloveDevice::Activate(uint32_t unObjectId)
{
    m_deviceId = unObjectId;
    m_isActiveInSteamVR = true;
    m_isConnected.exchange(true);
    m_isConnectedMainThreadLocal = true;
    m_ignorePoses.exchange(false);
    m_ignorePosesThreadLocal = false;

    m_ulProps = vr::VRProperties()->TrackedDeviceToPropertyContainer(m_deviceId);
    SetupProps();

    vr::VRDriverInput()->CreateHapticComponent(m_ulProps, "/output/haptic", &m_hapticInputHandle);

    // Compute default grip limit pose
    m_handSimulation.ComputeSkeletonTransforms(m_isLeft ? vr::TrackedControllerRole_LeftHand : vr::TrackedControllerRole_RightHand, {}, {}, m_handTransforms);

    vr::VRDriverInput()->CreateSkeletonComponent(
        m_ulProps,
        m_isLeft ? "/input/skeleton/left" : "/input/skeleton/right",
        m_isLeft ? "/skeleton/hand/left" : "/skeleton/hand/right",
        "/pose/raw",
        vr::EVRSkeletalTrackingLevel::VRSkeletalTracking_Full,
        m_handTransforms,
        NUM_BONES,
        &m_skeletalComponentHandle);

    // Update the skeleton immediately to inform SteamVR that we have a skeletal input capable device
    vr::VRDriverInput()->UpdateSkeletonComponent(m_skeletalComponentHandle, vr::VRSkeletalMotionRange_WithController,       m_handTransforms, NUM_BONES);
    vr::VRDriverInput()->UpdateSkeletonComponent(m_skeletalComponentHandle, vr::VRSkeletalMotionRange_WithoutController,    m_handTransforms, NUM_BONES);

    // Spawn input thread
    m_inputUpdateExecute.exchange(true);
    m_inputUpdateThread = std::thread(&ContactGloveDevice::InputUpdateThread, this);

    // Spawn pose thread
    m_poseUpdateExecute.exchange(true);
    m_poseUpdateThread = std::thread(&ContactGloveDevice::PoseUpdateThread, this);

    return vr::VRInitError_None;
}

void ContactGloveDevice::Deactivate()
{
    if (m_poseUpdateExecute.exchange(false)) {
        m_poseUpdateThread.join();
    }
    
    if (m_inputUpdateExecute.exchange(false)) {
        m_inputUpdateThread.join();
    }

    m_isActiveInSteamVR = false;
    m_isConnected.exchange(false);
    m_isConnectedMainThreadLocal = false;
    m_ignorePoses.exchange(true);
    m_ignorePosesThreadLocal = false;
}

void ContactGloveDevice::EnterStandby() {}

void* ContactGloveDevice::GetComponent(const char* pchComponentNameAndVersion) {
    return nullptr;
}

void ContactGloveDevice::DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize) {}

void ContactGloveDevice::Tick() {

    vr::VREvent_t vrEvent = {};
    while (vr::VRServerDriverHost()->PollNextEvent(&vrEvent, sizeof(vrEvent)))
    {
        switch (vrEvent.eventType) {
            case vr::VREvent_Input_HapticVibration: {
                if (vrEvent.data.hapticVibration.componentHandle == m_hapticInputHandle) {
                    
                    // This is where you would send a signal to your hardware to trigger actual haptic feedback
                    const float pulse_period    = 1.f / vrEvent.data.hapticVibration.fFrequency;
                    const float frequency       = Clamp(pulse_period,       1000000.f / 65535.f, 1000000.f / 300.f);
                    const float amplitude       = Clamp(vrEvent.data.hapticVibration.fAmplitude,        0.f, 1.f);
                    const float duration        = Clamp(vrEvent.data.hapticVibration.fDurationSeconds,  0.f, 10.f);
      
                    if(duration == 0.f) {
                        // Trigger a single pulse of the haptic component
                    } else {
                        const float pulse_count = vrEvent.data.hapticVibration.fDurationSeconds * vrEvent.data.hapticVibration.fFrequency;
                        // const float pulse_duration = Lerp(HAPTIC_MINIMUM_DURATION, HAPTIC_MAXIMUM_DURATION, amplitude);
                        // const float pulse_interval = pulse_period - pulse_duration;
                    }
                }
            }
            break;
        }
    }
}

// Creates index controller props
void ContactGloveDevice::SetupProps() {

    // @TODO: Gloves props, we want to use device emulation for VRChat etc

    // Props taken from
    // https://github.com/LucidVR/opengloves-driver/blob/v0.5.2/src/DeviceDriver/KnuckleDriver.cpp#L39
    // vr::VRProperties()->SetInt32Property(m_ulProps,			vr::Prop_ControllerHandSelectionPriority_Int32,		2147483647); // @FIXME: is this even necessary????
    vr::VRProperties()->SetStringProperty(m_ulProps,		vr::Prop_SerialNumber_String,						m_serial.c_str());
    vr::VRProperties()->SetBoolProperty(m_ulProps,			vr::Prop_WillDriftInYaw_Bool,						false);
    vr::VRProperties()->SetBoolProperty(m_ulProps,			vr::Prop_DeviceIsWireless_Bool,						true);
    vr::VRProperties()->SetBoolProperty(m_ulProps,			vr::Prop_DeviceIsCharging_Bool,						false); // We do not have a way to determine this it seems
    vr::VRProperties()->SetBoolProperty(m_ulProps,			vr::Prop_DeviceProvidesBatteryStatus_Bool,			true);
    vr::VRProperties()->SetFloatProperty(m_ulProps,			vr::Prop_DeviceBatteryPercentage_Float,				1.f); // Initialise with 100% battery, it'll get updated near instantly anyway

    vr::HmdMatrix34_t l_matrix = { -1.f, 0.f, 0.f, 0.f, 0.f, 0.f, -1.f, 0.f, 0.f, -1.f, 0.f, 0.f };
    vr::VRProperties()->SetProperty(m_ulProps,				vr::Prop_StatusDisplayTransform_Matrix34, &l_matrix, sizeof(vr::HmdMatrix34_t), vr::k_unHmdMatrix34PropertyTag);

    vr::VRProperties()->SetBoolProperty(m_ulProps,			vr::Prop_Firmware_UpdateAvailable_Bool,				false);
    vr::VRProperties()->SetBoolProperty(m_ulProps,			vr::Prop_Firmware_ManualUpdate_Bool,				false);
    vr::VRProperties()->SetStringProperty(m_ulProps,		vr::Prop_Firmware_ManualUpdateURL_String,			"https://developer.valvesoftware.com/wiki/SteamVR/HowTo_Update_Firmware");
    vr::VRProperties()->SetBoolProperty(m_ulProps,			vr::Prop_DeviceCanPowerOff_Bool,					false);
    vr::VRProperties()->SetInt32Property(m_ulProps,			vr::Prop_DeviceClass_Int32,							vr::TrackedDeviceClass_Controller);
    vr::VRProperties()->SetBoolProperty(m_ulProps,			vr::Prop_Firmware_ForceUpdateRequired_Bool,			false);
    vr::VRProperties()->SetBoolProperty(m_ulProps,			vr::Prop_Identifiable_Bool,							true);
    vr::VRProperties()->SetBoolProperty(m_ulProps,			vr::Prop_Firmware_RemindUpdate_Bool,				false);
    vr::VRProperties()->SetInt32Property(m_ulProps,			vr::Prop_Axis0Type_Int32,							vr::k_eControllerAxis_TrackPad);
    vr::VRProperties()->SetInt32Property(m_ulProps,			vr::Prop_Axis1Type_Int32,							vr::k_eControllerAxis_Trigger);
    vr::VRProperties()->SetInt32Property(m_ulProps,			vr::Prop_ControllerRoleHint_Int32,					m_isLeft ? vr::TrackedControllerRole_LeftHand : vr::TrackedControllerRole_RightHand);
    vr::VRProperties()->SetBoolProperty(m_ulProps,			vr::Prop_HasDisplayComponent_Bool,					false);
    vr::VRProperties()->SetBoolProperty(m_ulProps,			vr::Prop_HasCameraComponent_Bool,					false);
    vr::VRProperties()->SetBoolProperty(m_ulProps,			vr::Prop_HasDriverDirectModeComponent_Bool,			false);
    vr::VRProperties()->SetBoolProperty(m_ulProps,			vr::Prop_HasVirtualDisplayComponent_Bool,			false);
    vr::VRProperties()->SetInt32Property(m_ulProps,			vr::Prop_ControllerHandSelectionPriority_Int32,		2147483647);
    vr::VRProperties()->SetStringProperty(m_ulProps,		vr::Prop_ModelNumber_String,						m_isLeft ? "Knuckles Left" : "Knuckles Right");
    vr::VRProperties()->SetStringProperty(m_ulProps,		vr::Prop_RenderModelName_String,					m_isLeft ? "{indexcontroller}valve_controller_knu_1_0_left" : "{indexcontroller}valve_controller_knu_1_0_right");
    vr::VRProperties()->SetStringProperty(m_ulProps,		vr::Prop_ManufacturerName_String,					m_deviceManufacturer.c_str());
    vr::VRProperties()->SetStringProperty(m_ulProps,		vr::Prop_TrackingFirmwareVersion_String,			"1562916277 watchman@ValveBuilder02 2019-07-12 FPGA 538(2.26/10/2) BL 0 VRC 1562916277 Radio 1562882729");
    vr::VRProperties()->SetStringProperty(m_ulProps,		vr::Prop_HardwareRevision_String,					"product 17 rev 14.1.9 lot 2019/4/20 0");
    vr::VRProperties()->SetStringProperty(m_ulProps,		vr::Prop_ConnectedWirelessDongle_String,			"C2F75F5986-DIY");
    vr::VRProperties()->SetUint64Property(m_ulProps,		vr::Prop_HardwareRevision_Uint64,					286130441U);
    vr::VRProperties()->SetUint64Property(m_ulProps,		vr::Prop_FirmwareVersion_Uint64,					1562916277U);
    vr::VRProperties()->SetUint64Property(m_ulProps,		vr::Prop_FPGAVersion_Uint64,						538U);
    vr::VRProperties()->SetUint64Property(m_ulProps,		vr::Prop_VRCVersion_Uint64,							1562916277U);
    vr::VRProperties()->SetUint64Property(m_ulProps,		vr::Prop_RadioVersion_Uint64,						1562882729U);
    vr::VRProperties()->SetUint64Property(m_ulProps,		vr::Prop_DongleVersion_Uint64,						1558748372U);
    vr::VRProperties()->SetStringProperty(m_ulProps,		vr::Prop_Firmware_ProgrammingTarget_String,			m_isLeft ? "LHR-E217CD00" : "LHR-E217CD01");
    vr::VRProperties()->SetStringProperty(m_ulProps,		vr::Prop_ResourceRoot_String,						"indexcontroller");
    vr::VRProperties()->SetStringProperty(m_ulProps,		vr::Prop_RegisteredDeviceType_String,				m_isLeft ? "valve/index_controllerLHR-E217CD00" : "valve/index_controllerLHR-E217CD01");
    vr::VRProperties()->SetStringProperty(m_ulProps,		vr::Prop_InputProfilePath_String,					"{indexcontroller}/input/index_controller_profile.json");
    vr::VRProperties()->SetStringProperty(m_ulProps,		vr::Prop_NamedIconPathDeviceOff_String,				m_isLeft ? "{freescuba}/icons/left_controller_status_off.png"				: "{freescuba}/icons/right_controller_status_off.png");
    vr::VRProperties()->SetStringProperty(m_ulProps,		vr::Prop_NamedIconPathDeviceSearching_String,		m_isLeft ? "{freescuba}/icons/left_controller_status_searching.gif"			: "{freescuba}/icons/right_controller_status_searching.gif");
    vr::VRProperties()->SetStringProperty(m_ulProps,		vr::Prop_NamedIconPathDeviceSearchingAlert_String,	m_isLeft ? "{freescuba}/icons/left_controller_status_searching_alert.gif"	: "{freescuba}/icons/right_controller_status_searching_alert.gif");
    vr::VRProperties()->SetStringProperty(m_ulProps,		vr::Prop_NamedIconPathDeviceReady_String,			m_isLeft ? "{freescuba}/icons/left_controller_status_ready.png"				: "{freescuba}/icons/right_controller_status_ready.png");
    vr::VRProperties()->SetStringProperty(m_ulProps,		vr::Prop_NamedIconPathDeviceReadyAlert_String,		m_isLeft ? "{freescuba}/icons/left_controller_status_ready_alert.png"		: "{freescuba}/icons/right_controller_status_ready_alert.png");
    vr::VRProperties()->SetStringProperty(m_ulProps,		vr::Prop_NamedIconPathDeviceNotReady_String,		m_isLeft ? "{freescuba}/icons/left_controller_status_error.png"				: "{freescuba}/icons/right_controller_status_error.png");
    vr::VRProperties()->SetStringProperty(m_ulProps,		vr::Prop_NamedIconPathDeviceStandby_String,			m_isLeft ? "{freescuba}/icons/left_controller_status_off.png"				: "{freescuba}/icons/right_controller_status_off.png");
    vr::VRProperties()->SetStringProperty(m_ulProps,		vr::Prop_NamedIconPathDeviceAlertLow_String,		m_isLeft ? "{freescuba}/icons/left_controller_status_ready_low.png"			: "{freescuba}/icons/right_controller_status_ready_low.png");

    vr::VRProperties()->SetInt32Property(m_ulProps,			vr::Prop_Axis2Type_Int32,                           vr::k_eControllerAxis_Trigger);
    vr::VRProperties()->SetStringProperty(m_ulProps,		vr::Prop_ControllerType_String,                     "knuckles");

    vr::VRDriverInput()->CreateBooleanComponent(m_ulProps,	"/input/system/click",		&m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::SystemClick)]);
    vr::VRDriverInput()->CreateBooleanComponent(m_ulProps,	"/input/system/touch",		&m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::SystemTouch)]);
    vr::VRDriverInput()->CreateBooleanComponent(m_ulProps,	"/input/trigger/click",		&m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::TriggerClick)]);
    vr::VRDriverInput()->CreateScalarComponent( m_ulProps,	"/input/trigger/value",		&m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::TriggerValue)],	vr::VRScalarType_Absolute, vr::VRScalarUnits_NormalizedOneSided);
    vr::VRDriverInput()->CreateScalarComponent( m_ulProps,	"/input/trackpad/x",		&m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::TrackpadX)],		vr::VRScalarType_Absolute, vr::VRScalarUnits_NormalizedTwoSided);
    vr::VRDriverInput()->CreateScalarComponent( m_ulProps,	"/input/trackpad/y",		&m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::TrackpadY)],		vr::VRScalarType_Absolute, vr::VRScalarUnits_NormalizedTwoSided);
    vr::VRDriverInput()->CreateBooleanComponent(m_ulProps,	"/input/trackpad/touch",	&m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::TrackpadTouch)]);
    vr::VRDriverInput()->CreateScalarComponent( m_ulProps,	"/input/trackpad/force",	&m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::TrackpadForce)],	vr::VRScalarType_Absolute, vr::VRScalarUnits_NormalizedOneSided);
    vr::VRDriverInput()->CreateBooleanComponent(m_ulProps,	"/input/grip/touch",		&m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::GripTouch)]);
    vr::VRDriverInput()->CreateScalarComponent( m_ulProps,	"/input/grip/value",		&m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::GripValue)],		vr::VRScalarType_Absolute, vr::VRScalarUnits_NormalizedOneSided);
    vr::VRDriverInput()->CreateScalarComponent( m_ulProps,	"/input/grip/force",		&m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::GripForce)],		vr::VRScalarType_Absolute, vr::VRScalarUnits_NormalizedOneSided);
    vr::VRDriverInput()->CreateBooleanComponent(m_ulProps,	"/input/thumbstick/click",	&m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::ThumbstickClick)]);
    vr::VRDriverInput()->CreateBooleanComponent(m_ulProps,	"/input/thumbstick/touch",	&m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::ThumbstickTouch)]);
    vr::VRDriverInput()->CreateScalarComponent( m_ulProps,	"/input/thumbstick/x",		&m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::ThumbstickX)],	vr::VRScalarType_Absolute, vr::VRScalarUnits_NormalizedTwoSided);
    vr::VRDriverInput()->CreateScalarComponent( m_ulProps,	"/input/thumbstick/y",		&m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::ThumbstickY)],	vr::VRScalarType_Absolute, vr::VRScalarUnits_NormalizedTwoSided);
    vr::VRDriverInput()->CreateBooleanComponent(m_ulProps,	"/input/a/click",			&m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::AClick)]);
    vr::VRDriverInput()->CreateBooleanComponent(m_ulProps,	"/input/a/touch",			&m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::ATouch)]);
    vr::VRDriverInput()->CreateBooleanComponent(m_ulProps,	"/input/b/click",			&m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::BClick)]);
    vr::VRDriverInput()->CreateBooleanComponent(m_ulProps,	"/input/b/touch",			&m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::BTouch)]);
    vr::VRDriverInput()->CreateScalarComponent( m_ulProps,	"/input/finger/thumb",		&m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::FingerThumb)],	    vr::VRScalarType_Absolute, vr::VRScalarUnits_NormalizedTwoSided);
    vr::VRDriverInput()->CreateScalarComponent( m_ulProps,	"/input/finger/index",		&m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::FingerIndex)],	    vr::VRScalarType_Absolute, vr::VRScalarUnits_NormalizedTwoSided);
    vr::VRDriverInput()->CreateScalarComponent( m_ulProps,	"/input/finger/middle",		&m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::FingerMiddle)],	vr::VRScalarType_Absolute, vr::VRScalarUnits_NormalizedTwoSided);
    vr::VRDriverInput()->CreateScalarComponent( m_ulProps,	"/input/finger/ring",		&m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::FingerRing)],	    vr::VRScalarType_Absolute, vr::VRScalarUnits_NormalizedTwoSided);
    vr::VRDriverInput()->CreateScalarComponent( m_ulProps,	"/input/finger/pinky",		&m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::FingerPinky)],	    vr::VRScalarType_Absolute, vr::VRScalarUnits_NormalizedTwoSided);
}

// Input has to be fed at 90Hz, otherwise some applications will freak out and interpret ghost inputs
void ContactGloveDevice::InputUpdateThread() {
    LOG("Executing input thread for %s...", m_serial.c_str());

    while (m_inputUpdateExecute) {
        m_doInput.exchange(true);
        ContactGloveDevice::UpdateInputs(m_lastState);
        m_doInput.exchange(false);

        std::this_thread::sleep_for(std::chrono::milliseconds( (uint64_t) INPUT_FREQUENCY ));
    }

    LOG("Closing input thread for %s...", m_serial.c_str());
}

void ContactGloveDevice::PoseUpdateThread() {
    LOG("Executing pose thread for %s...", m_serial.c_str());

    while (m_poseUpdateExecute) {
        if (m_shadowDevice != CONTACT_GLOVE_INVALID_DEVICE_ID) {
            if (m_isConnected) {
                vr::DriverPose_t driverPose = vr::DriverPose_t();
                if (m_ignorePoses) {
                    driverPose = m_lastPose;
                } else {
                    driverPose = m_devProvider->GetCachedPose(m_shadowDevice);
                    vr::DriverPose_t refPose = driverPose;
                    m_lastPose = driverPose;

                    const vr::HmdVector3d_t refPosition = { refPose.vecPosition[0], refPose.vecPosition[1], refPose.vecPosition[2] };
                    const vr::HmdVector3d_t newPosition = refPosition + (m_poseOffset.pos * refPose.qRotation);

                    // Align the position, by offseting our offset by the current tracker rotation then offsetting by it's position
                    driverPose.vecPosition[0] = newPosition.v[0];
                    driverPose.vecPosition[1] = newPosition.v[1];
                    driverPose.vecPosition[2] = newPosition.v[2];

                    // Align the rotation by doing rotation composition
                    driverPose.qRotation = refPose.qRotation * m_poseOffset.rot;
                }

                vr::VRServerDriverHost()->TrackedDevicePoseUpdated(m_deviceId, driverPose, sizeof(vr::DriverPose_t));
            } else {
                vr::DriverPose_t driverPoseInvalid = vr::DriverPose_t();
                driverPoseInvalid.deviceIsConnected = false; // Set it to disconnected
                vr::VRServerDriverHost()->TrackedDevicePoseUpdated(m_deviceId, driverPoseInvalid, sizeof(vr::DriverPose_t));
            }
        } else if (m_isConnected) {
            vr::DriverPose_t driverPoseInvalid = vr::DriverPose_t();
            driverPoseInvalid.deviceIsConnected = false; // Set it to disconnected
            vr::VRServerDriverHost()->TrackedDevicePoseUpdated(m_deviceId, driverPoseInvalid, sizeof(vr::DriverPose_t));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    LOG("Closing pose thread for %s...", m_serial.c_str());
}

vr::DriverPose_t ContactGloveDevice::GetPose() {
    return vr::DriverPose_t();
}

void ContactGloveDevice::Update(const protocol::ContactGloveState_t& updateState) {

    // If the gloves have been connected for the first time, register them to SteamVR
    if (updateState.isConnected) {
        if (!m_isConnected) {
            // First frame where the glove has been connected. Add it to SteamVR
            LOG("Glove %s connected! Magnetra: %d", m_serial.c_str(), updateState.hasMagnetra);
            vr::VRServerDriverHost()->TrackedDeviceAdded(m_serial.c_str(), vr::TrackedDeviceClass_Controller, this);
        } else {
            // Glove has already been added to SteamVR, update props etc
            m_shadowDevice = updateState.trackerIndex;

            // Copy input state
            m_doInput.exchange(true);
            memcpy(&m_lastState, &updateState, sizeof updateState);
            m_doInput.exchange(false);
            
            // Copy the pose offset
            memcpy(&m_poseOffset, &updateState.calibration.poseOffset, sizeof(m_poseOffset));
        }
    }

    // Sync connected state
    if (m_isConnectedMainThreadLocal != updateState.isConnected) {
        m_isConnected.exchange(updateState.isConnected);
        m_isConnectedMainThreadLocal = updateState.isConnected;
    }
}

// Approximates a curl value given a metacarpal bone, proximal bone and distal bone
float ContactGloveDevice::ApproximateSingleFingerCurl(HandSkeletonBone metacarpal, HandSkeletonBone distal) {

    vr::HmdVector4_t metacarpalPos  = m_handTransforms[static_cast<short>(metacarpal)].position;
    vr::HmdVector4_t proximalPos    = m_handTransforms[static_cast<short>(metacarpal) + 1].position;
    vr::HmdVector4_t distalPos      = m_handTransforms[static_cast<short>(distal)].position;

    // Compute absolute proximal position since bone positions are relative
    proximalPos.v[0] = metacarpalPos.v[0] + proximalPos.v[0];
    proximalPos.v[1] = metacarpalPos.v[1] + proximalPos.v[1];
    proximalPos.v[2] = metacarpalPos.v[2] + proximalPos.v[2];

    // Bone positions are relative, add them to compute the absolute position of the distal bone
    for (int i = metacarpal; i <= distal; i++) {
        vr::HmdVector4_t bonePosition = m_handTransforms[static_cast<short>(i)].position;
        distalPos.v[0] = distalPos.v[0] + bonePosition.v[0];
        distalPos.v[1] = distalPos.v[1] + bonePosition.v[1];
        distalPos.v[2] = distalPos.v[2] + bonePosition.v[2];
    }

    // Compute the direction from the metacarpal to the proximal and distal
    vr::HmdVector3_t proximalDir = {
        .v = {
            proximalPos.v[0] - metacarpalPos.v[0],
            proximalPos.v[1] - metacarpalPos.v[1],
            proximalPos.v[2] - metacarpalPos.v[2]
        }
    };

    vr::HmdVector3_t distalDir = {
        .v = {
            distalPos.v[0] - metacarpalPos.v[0],
            distalPos.v[1] - metacarpalPos.v[1],
            distalPos.v[2] - metacarpalPos.v[2]
        }
    };

    double distanceProximal = sqrt(
        proximalDir.v[0] * proximalDir.v[0] +
        proximalDir.v[1] * proximalDir.v[1] +
        proximalDir.v[2] * proximalDir.v[2]);

    double distanceDistal = sqrt(
        distalDir.v[0] * distalDir.v[0] +
        distalDir.v[1] * distalDir.v[1] +
        distalDir.v[2] * distalDir.v[2]);

    double dotProduct =
        distalDir.v[0] / distanceProximal * distalDir.v[0] / distanceDistal +
        distalDir.v[1] / distanceProximal * distalDir.v[1] / distanceDistal +
        distalDir.v[2] / distanceProximal * distalDir.v[2] / distanceDistal;

    // Now that we have the length we can compute the angle it's at
    double angle = acos(dotProduct);
    const double ninetyDeg = DegToRad(90.f);

    return Clamp((float) (angle / ninetyDeg), -1.f, 1.f);
}

// Approximates curl values from a skeletal input pose
void ContactGloveDevice::ApproximateCurls(const protocol::ContactGloveState_t& updateState) {

    if (updateState.useCurl) {
        m_curlThumb     = ApproximateSingleFingerCurl(HandSkeletonBone::kHandSkeletonBone_Thumb0,           HandSkeletonBone::kHandSkeletonBone_Thumb3);
        m_curlIndex     = ApproximateSingleFingerCurl(HandSkeletonBone::kHandSkeletonBone_IndexFinger1,     HandSkeletonBone::kHandSkeletonBone_IndexFinger4);
        m_curlMiddle    = ApproximateSingleFingerCurl(HandSkeletonBone::kHandSkeletonBone_MiddleFinger1,    HandSkeletonBone::kHandSkeletonBone_MiddleFinger4);
        m_curlRing      = ApproximateSingleFingerCurl(HandSkeletonBone::kHandSkeletonBone_RingFinger1,      HandSkeletonBone::kHandSkeletonBone_RingFinger4);
        m_curlPinky     = ApproximateSingleFingerCurl(HandSkeletonBone::kHandSkeletonBone_PinkyFinger1,     HandSkeletonBone::kHandSkeletonBone_PinkyFinger4);
    } else {
        m_curlThumb     = (float)(0.3 * updateState.thumbRoot   + 0.7 * updateState.thumbTip);
        m_curlIndex     = (float)(0.3 * updateState.indexRoot   + 0.7 * updateState.indexTip);
        m_curlMiddle    = (float)(0.3 * updateState.middleRoot  + 0.7 * updateState.middleTip);
        m_curlRing      = (float)(0.3 * updateState.ringRoot    + 0.7 * updateState.ringTip);
        m_curlPinky     = (float)(0.3 * updateState.pinkyRoot   + 0.7 * updateState.pinkyTip);
    }
}

void ContactGloveDevice::UpdateSkeletalInput(const protocol::ContactGloveState_t& updateState) {

    GloveFingerCurls curls = {
        .thumb = {
            .proximal   = Clamp(updateState.thumbRoot, -1.0f, 1.0f),
            .distal     = Clamp(updateState.thumbTip,  -1.0f, 1.0f)
        },
        .index = {
            .proximal   = Clamp(updateState.indexRoot, -1.0f, 1.0f),
            .distal     = Clamp(updateState.indexTip,  -1.0f, 1.0f)
        },
        .middle = {
            .proximal   = Clamp(updateState.middleRoot, -1.0f, 1.0f),
            .distal     = Clamp(updateState.middleTip,  -1.0f, 1.0f)
        },
        .ring = {
            .proximal   = Clamp(updateState.ringRoot, -1.0f, 1.0f),
            .distal     = Clamp(updateState.ringTip,  -1.0f, 1.0f)
        },
        .pinky = {
            .proximal   = Clamp(updateState.pinkyRoot, -1.0f, 1.0f),
            .distal     = Clamp(updateState.pinkyTip,  -1.0f, 1.0f)
        }
    };
    GloveFingerSplays splays = {};

    m_handSimulation.ComputeSkeletonTransforms(m_isLeft ? vr::TrackedControllerRole_LeftHand : vr::TrackedControllerRole_RightHand, curls, splays, m_handTransforms);

    vr::VRDriverInput()->UpdateSkeletonComponent(m_skeletalComponentHandle, vr::VRSkeletalMotionRange_WithController,    m_handTransforms, NUM_BONES);
    vr::VRDriverInput()->UpdateSkeletonComponent(m_skeletalComponentHandle, vr::VRSkeletalMotionRange_WithoutController, m_handTransforms, NUM_BONES);

    ApproximateCurls(updateState);
}

// Apply a threshold
void ContactGloveDevice::HandleGesture(ThresholdState& param, const protocol::ContactGloveState_t::CalibrationData_t::GestureThreshold_t& thresholds, const float value) {
    param.value = max(min((value - thresholds.deactivate) / (1.0f - thresholds.deactivate), 1.0f), 0.0f);

    if (param.isActive == false) {
        // If the value is below the threshold and we aren't active
        if (value > thresholds.activate) {
            param.isActive = true;
        }
    } else {
        // If the value is above the threshold and we are active
        if (value < thresholds.deactivate) {
            param.isActive = false;
        }
    }
}

void ContactGloveDevice::UpdateInputs(const protocol::ContactGloveState_t& updateState) {
    if (updateState.isConnected) {
        // Update battery percentage
        float gloveBattery = updateState.gloveBattery * 0.01f;
        vr::VRProperties()->SetFloatProperty(m_ulProps, vr::Prop_DeviceBatteryPercentage_Float, gloveBattery);

        // Handle skeletal input
        UpdateSkeletalInput(updateState);
        
        // Activate thresholds
        if (updateState.useCurl) {
            HandleGesture(m_thumbActivation, updateState.calibration.gestures.thumb, m_curlThumb);
            HandleGesture(m_triggerActivation, updateState.calibration.gestures.trigger, m_curlIndex);
            HandleGesture(m_gripActivation, updateState.calibration.gestures.grip, (m_curlMiddle + m_curlRing + m_curlPinky) / 3.0f);
        } else {
            HandleGesture(m_thumbActivation, updateState.calibration.gestures.thumb, updateState.thumbTip);
            HandleGesture(m_triggerActivation, updateState.calibration.gestures.trigger, updateState.indexTip);
            HandleGesture(m_gripActivation, updateState.calibration.gestures.grip, (updateState.middleTip + updateState.ringTip + updateState.pinkyTip) / 3.0f);
        }

        if (updateState.hasMagnetra) {
            // Update inputs only if magnetra is connected
            vr::VRDriverInput()->UpdateScalarComponent(m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::ThumbstickX)], updateState.joystickX, 0);
            vr::VRDriverInput()->UpdateScalarComponent(m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::ThumbstickY)], -updateState.joystickY, 0); // Flipped in SteamVR for some reason
            vr::VRDriverInput()->UpdateBooleanComponent(m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::ThumbstickClick)], updateState.joystickClick, 0);
            vr::VRDriverInput()->UpdateBooleanComponent(m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::ThumbstickTouch)], updateState.joystickClick, 0);

            vr::VRDriverInput()->UpdateBooleanComponent(m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::AClick)], updateState.buttonDown, 0);
            vr::VRDriverInput()->UpdateBooleanComponent(m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::ATouch)], updateState.buttonDown || m_thumbActivation.isActive, 0); // Thumb is also going to activate A touch
            vr::VRDriverInput()->UpdateBooleanComponent(m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::BClick)], updateState.buttonUp, 0);
            vr::VRDriverInput()->UpdateBooleanComponent(m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::BTouch)], updateState.buttonUp, 0);
            vr::VRDriverInput()->UpdateBooleanComponent(m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::SystemClick)], updateState.systemUp || updateState.systemDown, 0);
            vr::VRDriverInput()->UpdateBooleanComponent(m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::SystemTouch)], updateState.systemUp || updateState.systemDown, 0);
        } else {
            // Default values
            vr::VRDriverInput()->UpdateScalarComponent(m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::ThumbstickX)], 0, 0);
            vr::VRDriverInput()->UpdateScalarComponent(m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::ThumbstickY)], 0, 0);
            vr::VRDriverInput()->UpdateBooleanComponent(m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::ThumbstickClick)], false, 0);
            vr::VRDriverInput()->UpdateBooleanComponent(m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::ThumbstickTouch)], false, 0);

            vr::VRDriverInput()->UpdateBooleanComponent(m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::AClick)], false, 0);
            vr::VRDriverInput()->UpdateBooleanComponent(m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::ATouch)], m_thumbActivation.isActive, 0); // Thumb is also going to activate A touch
            vr::VRDriverInput()->UpdateBooleanComponent(m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::BClick)], false, 0);
            vr::VRDriverInput()->UpdateBooleanComponent(m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::BTouch)], false, 0);
            vr::VRDriverInput()->UpdateBooleanComponent(m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::SystemClick)], false, 0);
            vr::VRDriverInput()->UpdateBooleanComponent(m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::SystemTouch)], false, 0);
        }

        vr::VRDriverInput()->UpdateBooleanComponent(m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::TriggerClick)], m_triggerActivation.isActive, 0);
        vr::VRDriverInput()->UpdateScalarComponent(m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::TriggerValue)], m_triggerActivation.value, 0);
        // Grip value => pull?
        // Grip force => force
        vr::VRDriverInput()->UpdateScalarComponent(m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::GripValue)], m_gripActivation.value, 0);
        vr::VRDriverInput()->UpdateScalarComponent(m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::GripForce)], m_gripActivation.value, 0);
        // vr::VRDriverInput()->UpdateScalarComponent(m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::TrackpadForce)], m_thumbActivation.value, 0);
        // vr::VRDriverInput()->UpdateScalarComponent(m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::TrackpadX)], 0, 0);
        // vr::VRDriverInput()->UpdateScalarComponent(m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::TrackpadY)], m_thumbActivation.value * -1, 0);

        // Finger curl for knuckles emu to work
        if (updateState.useCurl) {
            vr::VRDriverInput()->UpdateScalarComponent(m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::FingerThumb)],   m_curlThumb, 0);
            vr::VRDriverInput()->UpdateScalarComponent(m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::FingerIndex)],   m_curlIndex, 0);
            vr::VRDriverInput()->UpdateScalarComponent(m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::FingerMiddle)],  m_curlMiddle, 0);
            vr::VRDriverInput()->UpdateScalarComponent(m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::FingerRing)],    m_curlRing, 0);
            vr::VRDriverInput()->UpdateScalarComponent(m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::FingerPinky)],   m_curlPinky, 0);
        }
        else {
            vr::VRDriverInput()->UpdateScalarComponent(m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::FingerThumb)],   updateState.thumbRoot, 0);
            vr::VRDriverInput()->UpdateScalarComponent(m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::FingerIndex)],   updateState.indexRoot, 0);
            vr::VRDriverInput()->UpdateScalarComponent(m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::FingerMiddle)],  updateState.middleRoot, 0);
            vr::VRDriverInput()->UpdateScalarComponent(m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::FingerRing)],    updateState.ringRoot, 0);
            vr::VRDriverInput()->UpdateScalarComponent(m_inputComponentHandles[static_cast<int>(KnuckleDeviceComponentIndex_t::FingerPinky)],   updateState.pinkyRoot, 0);
        }

        // Update the current input state
        if (m_ignorePosesThreadLocal != updateState.ignorePose) {
            m_ignorePoses.exchange(updateState.ignorePose);
            m_ignorePosesThreadLocal = updateState.ignorePose;
        }
    }
}