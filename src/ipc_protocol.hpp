#pragma once

#include <stdint.h>

#define FREESCUBA_PIPE_NAME "\\\\.\\pipe\\FreeScubaDriver"
constexpr uint32_t CONTACT_GLOVE_INVALID_DEVICE_ID = 0xFFFFFFFF;

#ifndef _OPENVR_DRIVER_API
#include <openvr.h>

namespace vr {

	struct DriverPose_t
	{
		/* Time offset of this pose, in seconds from the actual time of the pose,
		 * relative to the time of the PoseUpdated() call made by the driver.
		 */
		double poseTimeOffset;

		/* Generally, the pose maintained by a driver
		 * is in an inertial coordinate system different
		 * from the world system of x+ right, y+ up, z+ back.
		 * Also, the driver is not usually tracking the "head" position,
		 * but instead an internal IMU or another reference point in the HMD.
		 * The following two transforms transform positions and orientations
		 * to app world space from driver world space,
		 * and to HMD head space from driver local body space.
		 *
		 * We maintain the driver pose state in its internal coordinate system,
		 * so we can do the pose prediction math without having to
		 * use angular acceleration.  A driver's angular acceleration is generally not measured,
		 * and is instead calculated from successive samples of angular velocity.
		 * This leads to a noisy angular acceleration values, which are also
		 * lagged due to the filtering required to reduce noise to an acceptable level.
		 */
		vr::HmdQuaternion_t qWorldFromDriverRotation;
		double vecWorldFromDriverTranslation[3];

		vr::HmdQuaternion_t qDriverFromHeadRotation;
		double vecDriverFromHeadTranslation[3];

		/* State of driver pose, in meters and radians. */
		/* Position of the driver tracking reference in driver world space
		* +[0] (x) is right
		* +[1] (y) is up
		* -[2] (z) is forward
		*/
		double vecPosition[3];

		/* Velocity of the pose in meters/second */
		double vecVelocity[3];

		/* Acceleration of the pose in meters/second */
		double vecAcceleration[3];

		/* Orientation of the tracker, represented as a quaternion */
		vr::HmdQuaternion_t qRotation;

		/* Angular velocity of the pose in axis-angle
		* representation. The direction is the angle of
		* rotation and the magnitude is the angle around
		* that axis in radians/second. */
		double vecAngularVelocity[3];

		/* Angular acceleration of the pose in axis-angle
		* representation. The direction is the angle of
		* rotation and the magnitude is the angle around
		* that axis in radians/second^2. */
		double vecAngularAcceleration[3];

		ETrackingResult result;

		bool poseIsValid;
		bool willDriftInYaw;
		bool shouldApplyHeadModel;
		bool deviceIsConnected;
	};
}
#endif

namespace protocol {
	const uint32_t Version = 1;

	enum RequestType_t
	{
		RequestInvalid,
		RequestHandshake,
		RequestUpdateGloveLeftState,
		RequestUpdateGloveRightState,
		// Haptics?
		RequestDevicePose,
	};

	enum ResponseType_t
	{
		ResponseInvalid,
		ResponseHandshake,
		ResponseSuccess,
		ResponseDevicePose,
	};

	enum GloveDevice_t {
		LeftGlove,
		RightGlove,
		Dongle,
	};

	struct Protocol_t
	{
		uint32_t version = Version;
	};

	constexpr uint8_t GLOVE_BATTERY_INVALID = 0xFF;

	struct ContactGloveState_t {

		bool isConnected = false;
		bool ignorePose = false;
		bool useCurl = false;
		uint32_t trackerIndex = CONTACT_GLOVE_INVALID_DEVICE_ID;

		uint16_t thumbRootRaw;
		uint16_t thumbTipRaw;
		uint16_t indexRootRaw;
		uint16_t indexTipRaw;
		uint16_t middleRootRaw;
		uint16_t middleTipRaw;
		uint16_t ringRootRaw;
		uint16_t ringTipRaw;
		uint16_t pinkyRootRaw;
		uint16_t pinkyTipRaw;

		float thumbRoot;
		float thumbTip;
		float indexRoot;
		float indexTip;
		float middleRoot;
		float middleTip;
		float ringRoot;
		float ringTip;
		float pinkyRoot;
		float pinkyTip;

		bool hasMagnetra;
		bool systemUp;
		bool systemDown;
		bool buttonUp;
		bool buttonDown;
		bool joystickClick;
		uint16_t joystickXRaw;
		uint16_t joystickYRaw;
		float joystickX;
		float joystickY;
		// No deadzone
		float joystickXUnfiltered;
		float joystickYUnfiltered;

		uint8_t gloveBatteryRaw;
		uint8_t gloveBattery;

		uint8_t firmwareMajor;
		uint8_t firmwareMinor;

		// Calibration for a single joint on the fingers
		struct FingerJointCalibrationData_t {
			uint16_t rest;	// Rest pose
			uint16_t bend;	// Bend pose (fingers bent away from the palm, backwards)
			uint16_t close; // Close pose (fingers bent towards the palm)
		};

		struct FingerCalibrationData_t {
			FingerJointCalibrationData_t proximal;
			FingerJointCalibrationData_t distal;
		};

		// Joint collection forming an entire hand
		struct HandFingersCalibrationData_t {
			/*
			FingerJointCalibrationData_t thumbRoot;
			FingerJointCalibrationData_t thumbTip;
			FingerJointCalibrationData_t indexRoot;
			FingerJointCalibrationData_t indexTip;
			FingerJointCalibrationData_t middleRoot;
			FingerJointCalibrationData_t middleTip;
			FingerJointCalibrationData_t ringRoot;
			FingerJointCalibrationData_t ringTip;
			FingerJointCalibrationData_t pinkyRoot;
			FingerJointCalibrationData_t pinkyTip;
			*/
			FingerCalibrationData_t thumb;
			FingerCalibrationData_t index;
			FingerCalibrationData_t middle;
			FingerCalibrationData_t ring;
			FingerCalibrationData_t pinky;
		};

		struct CalibrationData_t {
			struct JoystickCalibration_t {
				float threshold;
				// Bounds for joystick
				uint16_t XMax;
				uint16_t XMin;
				uint16_t YMax;
				uint16_t YMin;
				uint16_t XNeutral;
				uint16_t YNeutral;
				float forwardAngle;
			} joystick;

			struct PoseOffset_t {
				vr::HmdVector3d_t pos;
				vr::HmdQuaternion_t rot;
			} poseOffset;

			HandFingersCalibrationData_t fingers;

			struct GestureThreshold_t {
				float activate;
				float deactivate;
			};

			struct GestureCalibration_t {
				GestureThreshold_t thumb;
				GestureThreshold_t trigger;
				GestureThreshold_t grip;
			} gestures;
			
		} calibration;
	};

	struct Request_t
	{
		RequestType_t type;

		union {
			ContactGloveState_t gloveData;
			uint32_t driverPoseIndex;
		};

		Request_t()												: type(RequestType_t::RequestInvalid), gloveData{} { }
		Request_t(RequestType_t type)							: type(type), gloveData{} { }
		Request_t(ContactGloveState_t params, bool leftHand)	: type(leftHand ? RequestType_t::RequestUpdateGloveLeftState : RequestType_t::RequestUpdateGloveRightState), gloveData(params) {}
		Request_t(uint32_t driverPoseIndex)						: type(RequestType_t::RequestDevicePose),	driverPoseIndex(driverPoseIndex) {}
	};

	struct Response_t
	{
		ResponseType_t type;

		union {
			Protocol_t protocol;
			vr::DriverPose_t driverPose;
		};

		Response_t()											: type(ResponseType_t::ResponseInvalid), protocol{} { }
		Response_t(ResponseType_t type)							: type(type), protocol{} { }
		Response_t(vr::DriverPose_t pose)						: type(ResponseType_t::ResponseDevicePose), driverPose(pose){ }
	};
}