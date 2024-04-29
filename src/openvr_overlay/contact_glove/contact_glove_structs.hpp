#pragma once

#include <cinttypes>

/*

DONGLE, NO GLOVE:

Received data (CRC: 0x0):: uint8_t packet[9] = { 0x00, 0x07, 0x01, 0x06, 0x01, 0x06, 0x01, 0x06, 0x1D };
Received data (CRC: 0x0):: uint8_t packet[8] = { 0x00, 0x01, 0x3F, 0x00, 0x90, 0x00, 0x84, 0x73 };
Received data (CRC: 0x0):: uint8_t packet[8] = { 0x00, 0x02, 0x3F, 0x00, 0x98, 0x00, 0x89, 0x7A };
Received data (CRC: 0x0):: uint8_t packet[8] = { 0x00, 0x1E, 0x62, 0x5F, 0x5C, 0x5F, 0x01, 0x77 };

RIGHT GLOVE:

Received data (CRC: 0x0):: uint8_t packet[23] = { 0x00, 0x05, 0x35, 0x06, 0xF9, 0x06, 0xA7, 0x06, 0x30, 0x06, 0xE0, 0x06, 0x9C, 0x06, 0x0A, 0x07, 0xB2, 0x06, 0x58, 0x06, 0x1F, 0x06, 0x89 };
Received data (CRC: 0x0):: uint8_t packet[11] = { 0x00, 0x0B, 0xFF, 0x00, 0x60, 0x89, 0x1C, 0x73, 0x21, 0x80, 0x57 };

LEFT GLOVE:

Received data (CRC: 0x0):: uint8_t packet[23] = { 0x00, 0x04, 0xD3, 0x06, 0xF0, 0x06, 0xA0, 0x06, 0xD9, 0x06, 0xC5, 0x06, 0xD9, 0x06, 0xBA, 0x06, 0x5B, 0x06, 0x9C, 0x06, 0x94, 0x06, 0x1F };
Received data (CRC: 0x0):: uint8_t packet[11] = { 0x00, 0x0A, 0x30, 0x00, 0x58, 0x82, 0x92, 0x86, 0xDC, 0x7F, 0xAE };

DEVICE POWERED ON:
Received data (CRC: 0x0):: uint8_t unknown_packet[7] = { 0x00, 0x64, 0x01, 0x10, 0x01, 0x00, 0x82 };

*/

// 254 is the maximum size of a COBS frame
constexpr uint8_t MAX_PACKET_SIZE							= 254;

constexpr uint8_t DEVICES_VERSIONS							= 0x07;
constexpr uint8_t DEVICES_STATUS							= 0x1E;
constexpr uint8_t GLOVE_POWER_ON_PACKET						= 0x64;

constexpr uint8_t GLOVE_RIGHT_PACKET_DATA					= 0x02; // 8  bytes
constexpr uint8_t GLOVE_RIGHT_PACKET_FINGERS				= 0x05; // 23 bytes
constexpr uint8_t GLOVE_RIGHT_PACKET_IMU					= 0x0B; // 11 bytes

constexpr uint8_t GLOVE_LEFT_PACKET_DATA					= 0x01; // 8  bytes
constexpr uint8_t GLOVE_LEFT_PACKET_FINGERS					= 0x04; // 23 bytes
constexpr uint8_t GLOVE_LEFT_PACKET_IMU						= 0x0A; // 11 bytes

constexpr uint8_t CONTACT_GLOVE_INPUT_MASK_SYSTEM_UP		= 0b00010000;
constexpr uint8_t CONTACT_GLOVE_INPUT_MASK_SYSTEM_DOWN		= 0b00001000;
constexpr uint8_t CONTACT_GLOVE_INPUT_MASK_BUTTON_UP		= 0b00000010;
constexpr uint8_t CONTACT_GLOVE_INPUT_MASK_BUTTON_DOWN		= 0b00000001;
constexpr uint8_t CONTACT_GLOVE_INPUT_MASK_JOYSTICK_CLICK	= 0b00000100;
constexpr uint8_t CONTACT_GLOVE_INPUT_MASK_MAGNETRA_PRESENT	= 0b00100000;

constexpr uint8_t CONTACT_GLOVE_INVALID_BATTERY				= 0xFF;
constexpr uint32_t GLOVE_BATTERY_THRESHOLD					= 5;

enum class ContactGloveDevice_t {
	LeftGlove,
	RightGlove,
	Dongle,
};

// ACTUAL STRUCTS
struct GloveInputData_t {
public:
	bool hasMagnetra;
	bool systemUp;
	bool systemDown;
	bool buttonUp;
	bool buttonDown;
	bool joystickClick;
	uint16_t joystickX;
	uint16_t joystickY;
};
struct DevicesFirmware_t {
public:
	uint8_t dongleMinor;
	uint8_t dongleMajor;
	uint8_t gloveLeftMinor;
	uint8_t gloveLeftMajor;
	uint8_t gloveRightMinor;
	uint8_t gloveRightMajor;
};
struct DevicesStatus_t {
public:
	uint8_t gloveLeftBattery;
	uint8_t gloveRightBattery;
};

struct GlovePacketFingers_t {
public:
	uint16_t fingerThumbTip;
	uint16_t fingerThumbRoot;
	uint16_t fingerIndexTip;
	uint16_t fingerIndexRoot;
	uint16_t fingerMiddleTip;
	uint16_t fingerMiddleRoot;
	uint16_t fingerRingTip;
	uint16_t fingerRingRoot;
	uint16_t fingerPinkyTip;
	uint16_t fingerPinkyRoot;
};

struct GlovePacketImu_t {
public:
	uint16_t imu1;
	uint16_t imu2;
	uint16_t imu3;
	uint16_t imu4;

	float imu5;
	float imu6;
};

enum class PacketType_t {
	DevicesFirmware,
	DevicesStatus,
	GlovePowerOn,

	GloveLeftData,
	GloveLeftFingers,
	GloveLeftImu,

	GloveRightData,
	GloveRightFingers,
	GloveRightImu,
};

// Common packet denominator
struct ContactGlovePacket_t {
public:
	PacketType_t type;
	union ContactGlovePacket {
		GloveInputData_t		gloveData;
		GlovePacketFingers_t	gloveFingers;
		GlovePacketImu_t		gloveImu;
		DevicesFirmware_t		firmware;
		DevicesStatus_t			status;
	} packet;
};