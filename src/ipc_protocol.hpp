#pragma once

#include <stdint.h>

#define FREESCUBA_PIPE_NAME "\\\\.\\pipe\\FreeScubaDriver"

namespace protocol {
	const uint32_t Version = 1;

	enum RequestType
	{
		RequestInvalid,
		RequestHandshake,
		RequestUpdateInput,
		RequestUpdateFingers,
		RequestUpdateGloveState,
		// Haptics?
	};

	enum ResponseType
	{
		ResponseInvalid,
		ResponseHandshake,
		ResponseSuccess,
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

	// A finger tracking update packet
	struct FingersUpdate {
		GloveDevice device;
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
	};

	struct InputUpdate {
		bool hasMagnetra;
		bool systemUp;
		bool systemDown;
		bool buttonUp;
		bool buttonDown;
		bool joystickClick;
		float joystickX;
		float joystickY;
	};

	constexpr uint8_t GLOVE_BATTERY_INVALID = 0xFF;

	struct StateUpdate {
		uint8_t gloveLeftBattery;
		uint8_t gloveRightBattery;
	};

	struct Request
	{
		RequestType type;

		union {
			FingersUpdate fingers;
			InputUpdate input;
			StateUpdate state;
		};

		Request() : type(RequestInvalid) { }
		Request(RequestType type)		: type(type) { }
		Request(FingersUpdate params)	: type(RequestType::RequestUpdateFingers),		fingers(params) {}
		Request(InputUpdate params)		: type(RequestType::RequestUpdateInput),		input(params) {}
		Request(StateUpdate params)		: type(RequestType::RequestUpdateGloveState),	state(params) {}
	};

	struct Response
	{
		ResponseType type;

		union {
			Protocol protocol;
		};

		Response() : type(ResponseInvalid) { }
		Response(ResponseType type) : type(type) { }
	};
}