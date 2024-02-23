#pragma once

#include "openvr_driver.h"

// Internal state of a glove as tracked by the driver
struct ContactGloveState {
	// Fingers
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
	// Input
	bool hasMagnetra;
	bool systemUp;
	bool systemDown;
	bool buttonUp;
	bool buttonDown;
	bool joystickClick;
	float joystickX;
	float joystickY;
	// Battery
	uint8_t battery;
};

// SteamVR tracked device
class ContactGloveDevice {

};