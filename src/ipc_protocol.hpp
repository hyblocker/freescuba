#pragma once

#include <stdint.h>

#define FREESCUBA_PIPE_NAME "\\\\.\\pipe\\FreeScubaDriver"
#define CONTACT_GLOVE_INVALID_DEVICE_ID (0xFFFFFFFF)

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

	enum RequestType
	{
		RequestInvalid,
		RequestHandshake,
		RequestUpdateGloveLeftState,
		RequestUpdateGloveRightState,
		// Haptics?
		RequestDevicePose,
	};

	enum ResponseType
	{
		ResponseInvalid,
		ResponseHandshake,
		ResponseSuccess,
		ResponseDevicePose,
	};

	enum GloveDevice {
		LeftGlove,
		RightGlove,
		Dongle,
	};

	struct Protocol
	{
		uint32_t version = Version;
	};

	#define GLOVE_BATTERY_INVALID 0xFF

	struct ContactGloveState {

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
		struct FingerJointCalibrationData {
			uint16_t rest;	// Rest pose
			uint16_t bend;	// Bend pose (fingers bent away from the palm, backwards)
			uint16_t close; // Close pose (fingers bent towards the palm)
		};
		// Joint collection forming an entire hand
		struct FingerCalibrationData {
			FingerJointCalibrationData thumbRoot;
			FingerJointCalibrationData thumbTip;
			FingerJointCalibrationData indexRoot;
			FingerJointCalibrationData indexTip;
			FingerJointCalibrationData middleRoot;
			FingerJointCalibrationData middleTip;
			FingerJointCalibrationData ringRoot;
			FingerJointCalibrationData ringTip;
			FingerJointCalibrationData pinkyRoot;
			FingerJointCalibrationData pinkyTip;
		};

		struct CalibrationData {
			struct JoystickCalibration {
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

			struct PoseOffset {
				vr::HmdVector3d_t pos;
				vr::HmdQuaternion_t rot;
			} poseOffset;

			FingerCalibrationData fingers;

			struct GestureThreshold {
				float activate;
				float deactivate;
			};

			struct GestureCalibration {
				GestureThreshold thumb;
				GestureThreshold trigger;
				GestureThreshold grip;
			} gestures;
			
		} calibration;
	};

	struct Request
	{
		RequestType type;

		union {
			ContactGloveState gloveData;
			uint32_t driverPoseIndex;
		};

		Request()											: type(RequestType::RequestInvalid), gloveData{} { }
		Request(RequestType type)							: type(type), gloveData{} { }
		Request(ContactGloveState params, bool leftHand)	: type(leftHand ? RequestType::RequestUpdateGloveLeftState : RequestType::RequestUpdateGloveRightState),	gloveData(params) {}
		Request(uint32_t driverPoseIndex)					: type(RequestType::RequestDevicePose),	driverPoseIndex(driverPoseIndex) {}
	};

	struct Response
	{
		ResponseType type;

		union {
			Protocol protocol;
			vr::DriverPose_t driverPose;
		};

		Response() : type(ResponseType::ResponseInvalid), protocol{} { }
		Response(ResponseType type) : type(type), protocol{} { }
		Response(vr::DriverPose_t pose) : type(ResponseType::ResponseDevicePose), driverPose(pose){ }
	};
}