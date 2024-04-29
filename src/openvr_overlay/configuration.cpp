#include "configuration.hpp"

#include <picojson.h>
#include <shlobj_core.h>
#include <locale>
#include <codecvt>
#include <fstream>

static std::wstring s_configPath;
static bool s_directoriesExist = false;

static bool EnsureDirectoriesExist() {
	PWSTR RootPath = NULL;
	if (S_OK != SHGetKnownFolderPath(FOLDERID_LocalAppDataLow, 0, NULL, &RootPath)) {
		CoTaskMemFree(RootPath);
		return false;
	}

	s_configPath = RootPath;
	CoTaskMemFree(RootPath);

	s_configPath += LR"(\FreeScuba)";
	if (CreateDirectoryW(s_configPath.c_str(), 0) == 0 && GetLastError() != ERROR_ALREADY_EXISTS) {
		return false;
	}
	s_configPath += L"\\config.json";

	return true;
}

inline void TryReadBool(bool& propToWriteTo, picojson::object objectToReadFrom, const char* valueName) {
	try {
		propToWriteTo = objectToReadFrom[valueName].get<bool>();
	} catch (std::runtime_error) {}
}
inline void TryReadDouble(double& propToWriteTo, picojson::object objectToReadFrom, const char* valueName) {
	try {
		propToWriteTo = objectToReadFrom[valueName].get<double>();
	} catch (std::runtime_error) {}
}
inline void TryReadFloat(float& propToWriteTo, picojson::object objectToReadFrom, const char* valueName) {
	try {
		propToWriteTo = (float) objectToReadFrom[valueName].get<double>();
	} catch (std::runtime_error) {}
}
inline void TryReadUint16(uint16_t& propToWriteTo, picojson::object objectToReadFrom, const char* valueName) {
	try {
		propToWriteTo = (uint16_t) objectToReadFrom[valueName].get<double>();
	} catch (std::runtime_error) {}
}

void ReadPoseOffset(protocol::ContactGloveState_t::CalibrationData_t::PoseOffset_t& state, picojson::object& jsonObj) {

	try {
		picojson::object trackerOffsetRoot = jsonObj["pose"].get<picojson::object>();

		try {
			picojson::object positionRoot = trackerOffsetRoot["position"].get<picojson::object>();
			TryReadDouble(state.pos.v[0], positionRoot, "x");
			TryReadDouble(state.pos.v[1], positionRoot, "y");
			TryReadDouble(state.pos.v[2], positionRoot, "z");
			// state.pos.v[0] = positionRoot["x"].get<double>();
			// state.pos.v[1] = positionRoot["y"].get<double>();
			// state.pos.v[2] = positionRoot["z"].get<double>();
		}catch (std::runtime_error) {}

		try {
			picojson::object rotationRoot = trackerOffsetRoot["rotation"].get<picojson::object>();
			TryReadDouble(state.rot.x, rotationRoot, "x");
			TryReadDouble(state.rot.y, rotationRoot, "y");
			TryReadDouble(state.rot.z, rotationRoot, "z");
			TryReadDouble(state.rot.w, rotationRoot, "w");
			// state.rot.x = rotationRoot["x"].get<double>();
			// state.rot.y = rotationRoot["y"].get<double>();
			// state.rot.z = rotationRoot["z"].get<double>();
			// state.rot.w = rotationRoot["w"].get<double>();
		}catch (std::runtime_error) {}
	}catch (std::runtime_error) {}
}

void ReadJoystickCalibration(protocol::ContactGloveState_t::CalibrationData_t::JoystickCalibration_t& state, picojson::object& jsonObj) {

	try {
		picojson::object joystickRoot = jsonObj["joystick"].get<picojson::object>();

		TryReadFloat(state.threshold,		joystickRoot, "threshold");
		TryReadFloat(state.forwardAngle,	joystickRoot, "forward");
		// state.threshold		= (float) joystickRoot["threshold"].get<double>();
		// state.forwardAngle	= (float) joystickRoot["forward"].get<double>();

		TryReadUint16(state.XMax,			joystickRoot, "xmax");
		TryReadUint16(state.XMin,			joystickRoot, "xmin");
		TryReadUint16(state.YMax,			joystickRoot, "ymax");
		TryReadUint16(state.YMin,			joystickRoot, "ymin");
		TryReadUint16(state.XNeutral,		joystickRoot, "xneutral");
		TryReadUint16(state.YNeutral,		joystickRoot, "yneutral");

		// state.XMax			= (uint16_t) joystickRoot["xmax"].get<double>();
		// state.XMin			= (uint16_t) joystickRoot["xmin"].get<double>();
		// state.YMax			= (uint16_t) joystickRoot["ymax"].get<double>();
		// state.YMin			= (uint16_t) joystickRoot["ymin"].get<double>();
		// state.XNeutral		= (uint16_t) joystickRoot["xneutral"].get<double>();
		// state.YNeutral		= (uint16_t) joystickRoot["yneutral"].get<double>();
	} catch (std::runtime_error) {}
}

void ReadFingerJointCalibration(protocol::ContactGloveState_t::FingerJointCalibrationData_t& state, picojson::object& jsonObj) {
	try {
		// state.rest		= (uint16_t) jsonObj["rest"].get<double>();
		// state.bend		= (uint16_t) jsonObj["bend"].get<double>();
		// state.close		= (uint16_t) jsonObj["close"].get<double>();

		TryReadUint16(state.rest,	jsonObj, "rest");
		TryReadUint16(state.bend,	jsonObj, "bend");
		TryReadUint16(state.close,	jsonObj, "close");
	} catch (std::runtime_error) {}
}

void ReadFingersCalibration(protocol::ContactGloveState_t::FingerCalibrationData_t& state, picojson::object& jsonObj) {

	try {
	picojson::object fingersRoot = jsonObj["fingers"].get<picojson::object>();

#define READ_FINGER_CALIBRATION(jointRoot)														\
	try {																						\
		picojson::object jointRoot##Json = fingersRoot[#jointRoot].get<picojson::object>();		\
		ReadFingerJointCalibration(state.jointRoot, jointRoot##Json);							\
	} catch (std::runtime_error) {}

	READ_FINGER_CALIBRATION(thumbRoot);
	READ_FINGER_CALIBRATION(thumbTip);
	READ_FINGER_CALIBRATION(indexRoot);
	READ_FINGER_CALIBRATION(indexTip);
	READ_FINGER_CALIBRATION(middleRoot);
	READ_FINGER_CALIBRATION(middleTip);
	READ_FINGER_CALIBRATION(ringRoot);
	READ_FINGER_CALIBRATION(ringTip);
	READ_FINGER_CALIBRATION(pinkyRoot);
	READ_FINGER_CALIBRATION(pinkyTip);

#undef READ_FINGER_CALIBRATION
	
	} catch (std::runtime_error) {}
}

void ReadGestures(protocol::ContactGloveState_t::CalibrationData_t::GestureCalibration_t& state, picojson::object& jsonObj) {

	try {
		picojson::object gesturesRoot = jsonObj["gestures"].get<picojson::object>();

		TryReadFloat(state.grip.activate, gesturesRoot, "grip_activate");
		TryReadFloat(state.grip.deactivate, gesturesRoot, "grip_deactivate");
		// state.grip.activate = (float)gesturesRoot["grip_activate"].get<double>();
		// state.grip.deactivate = (float)gesturesRoot["grip_deactivate"].get<double>();

		TryReadFloat(state.trigger.activate, gesturesRoot, "trigger_activate");
		TryReadFloat(state.trigger.deactivate, gesturesRoot, "trigger_deactivate");
		// state.trigger.activate = (float)gesturesRoot["trigger_activate"].get<double>();
		// state.trigger.deactivate = (float)gesturesRoot["trigger_deactivate"].get<double>();

		TryReadFloat(state.thumb.activate, gesturesRoot, "thumb_activate");
		TryReadFloat(state.thumb.deactivate, gesturesRoot, "thumb_deactivate");
		// state.thumb.activate = (float)gesturesRoot["thumb_activate"].get<double>();
		// state.thumb.deactivate = (float)gesturesRoot["thumb_deactivate"].get<double>();

	} catch (std::runtime_error) {}
}

void LoadConfiguration(AppState& state) {
	if (!s_directoriesExist) {
		s_directoriesExist = EnsureDirectoriesExist();
	}
	if (!s_directoriesExist) {
		throw std::exception("Failed to creare config directory. Aborting...");
	}

	// Loads the config file from disk
	std::ifstream fileStream(s_configPath);

	if (fileStream.is_open()) {
		// Wrap in try catch as we shouldn't crash on invalid config
		try {
			picojson::value v;
			std::string err = picojson::parse(v, fileStream);
			if (!err.empty())
				throw std::runtime_error(err);

			auto rootObj = v.get<picojson::object>();

			// Read automatic launch
			// try {
			// 	state.doAutoLaunch = rootObj["doAutoLaunch"].get<bool>();
			// } catch (std::runtime_error) {}
			TryReadBool(state.doAutoLaunch, rootObj, "doAutoLaunch");

			// Load left glove config
			try {
				auto leftGloveObj = rootObj["left"].get<picojson::object>();
		
				ReadPoseOffset(state.gloveLeft.calibration.poseOffset, leftGloveObj);
				ReadJoystickCalibration(state.gloveLeft.calibration.joystick, leftGloveObj);
				ReadFingersCalibration(state.gloveLeft.calibration.fingers, leftGloveObj);
				ReadGestures(state.gloveLeft.calibration.gestures, leftGloveObj);
			} catch (std::runtime_error) {}

			// Load right glove config
			try {
				auto rightGloveObj = rootObj["right"].get<picojson::object>();
		
				ReadPoseOffset(state.gloveRight.calibration.poseOffset, rightGloveObj);
				ReadJoystickCalibration(state.gloveRight.calibration.joystick, rightGloveObj);
				ReadFingersCalibration(state.gloveRight.calibration.fingers, rightGloveObj);
				ReadGestures(state.gloveRight.calibration.gestures, rightGloveObj);
			} catch (std::runtime_error) {}

		} catch (std::runtime_error){}
		
		fileStream.close();
	}
}

void WritePoseCalibration(protocol::ContactGloveState_t::CalibrationData_t::PoseOffset_t& state, picojson::object& jsonObj) {

	picojson::object trackerOffsetRoot;
	picojson::object positionRoot;
	picojson::object rotationRoot;

	positionRoot["x"].set<double>(state.pos.v[0]);
	positionRoot["y"].set<double>(state.pos.v[1]);
	positionRoot["z"].set<double>(state.pos.v[2]);

	rotationRoot["x"].set<double>(state.rot.x);
	rotationRoot["y"].set<double>(state.rot.y);
	rotationRoot["z"].set<double>(state.rot.z);
	rotationRoot["w"].set<double>(state.rot.w);

	trackerOffsetRoot["position"].set<picojson::object>(positionRoot);
	trackerOffsetRoot["rotation"].set<picojson::object>(rotationRoot);

	jsonObj["pose"].set<picojson::object>(trackerOffsetRoot);
}

void WriteJoystickCalibration(protocol::ContactGloveState_t::CalibrationData_t::JoystickCalibration_t& state, picojson::object& jsonObj) {

	picojson::object joystickRoot;

	double buf = state.threshold; joystickRoot["threshold"].set<double>( buf );
	buf = state.XMax; joystickRoot["xmax"].set<double>( buf );
	buf = state.XMin; joystickRoot["xmin"].set<double>( buf );
	buf = state.YMax; joystickRoot["ymax"].set<double>( buf );
	buf = state.YMin; joystickRoot["ymin"].set<double>( buf );
	buf = state.XNeutral; joystickRoot["xneutral"].set<double>( buf );
	buf = state.YNeutral; joystickRoot["yneutral"].set<double>( buf );
	buf = state.forwardAngle; joystickRoot["forward"].set<double>( buf );

	jsonObj["joystick"].set<picojson::object>(joystickRoot);
}

void WriteFingerJointCalibration(protocol::ContactGloveState_t::FingerJointCalibrationData_t& state, picojson::object& jsonObj) {
	double buf = state.rest;
	jsonObj["rest"].set<double>( buf );
	buf = state.bend;
	jsonObj["bend"].set<double>( buf );
	buf = state.close;
	jsonObj["close"].set<double>( buf );
}

void WriteFingersCalibration(protocol::ContactGloveState_t::FingerCalibrationData_t& state, picojson::object& jsonObj) {

	picojson::object fingersRoot;

#define WRITE_FINGER_CALIBRATION(jointRoot)							\
	picojson::object jointRoot##Json;								\
	WriteFingerJointCalibration(state.jointRoot, jointRoot##Json);	\
	fingersRoot[#jointRoot].set<picojson::object>(jointRoot##Json);

	WRITE_FINGER_CALIBRATION(thumbRoot);
	WRITE_FINGER_CALIBRATION(thumbTip);
	WRITE_FINGER_CALIBRATION(indexRoot);
	WRITE_FINGER_CALIBRATION(indexTip);
	WRITE_FINGER_CALIBRATION(middleRoot);
	WRITE_FINGER_CALIBRATION(middleTip);
	WRITE_FINGER_CALIBRATION(ringRoot);
	WRITE_FINGER_CALIBRATION(ringTip);
	WRITE_FINGER_CALIBRATION(pinkyRoot);
	WRITE_FINGER_CALIBRATION(pinkyTip);

#undef WRITE_FINGER_CALIBRATION

	jsonObj["fingers"].set<picojson::object>(fingersRoot);
}

void WriteThresholds(protocol::ContactGloveState_t::CalibrationData_t::GestureCalibration_t& state, picojson::object& jsonObj) {

	picojson::object gesturesRoot;

	double buf = state.grip.activate;
	gesturesRoot["grip_activate"].set<double>(buf);
	buf = state.grip.deactivate;
	gesturesRoot["grip_deactivate"].set<double>(buf);

	buf = state.trigger.activate;
	gesturesRoot["trigger_activate"].set<double>(buf);
	buf = state.trigger.deactivate;
	gesturesRoot["trigger_deactivate"].set<double>(buf);

	buf = state.thumb.activate;
	gesturesRoot["thumb_activate"].set<double>(buf);
	buf = state.thumb.deactivate;
	gesturesRoot["thumb_deactivate"].set<double>(buf);

	jsonObj["gestures"].set<picojson::object>(gesturesRoot);
}

void SaveConfiguration(AppState& state) {
	if (!s_directoriesExist) {
		s_directoriesExist = EnsureDirectoriesExist();
	}
	if (!s_directoriesExist) {
		throw std::exception("Failed to creare config directory. Aborting...");
	}

	// Saves the config file to disk
	std::ofstream fileStream(s_configPath);
	if (fileStream.is_open()) {

		picojson::object config;

		picojson::object gloveLeftConfig;

		// Write props
		WritePoseCalibration(state.gloveLeft.calibration.poseOffset, gloveLeftConfig);
		WriteJoystickCalibration(state.gloveLeft.calibration.joystick, gloveLeftConfig);
		WriteFingersCalibration(state.gloveLeft.calibration.fingers, gloveLeftConfig);
		WriteThresholds(state.gloveLeft.calibration.gestures, gloveLeftConfig);

		picojson::object gloveRightConfig;
		
		// Write props
		WritePoseCalibration(state.gloveRight.calibration.poseOffset, gloveRightConfig);
		WriteJoystickCalibration(state.gloveRight.calibration.joystick, gloveRightConfig);
		WriteFingersCalibration(state.gloveRight.calibration.fingers, gloveRightConfig);
		WriteThresholds(state.gloveRight.calibration.gestures, gloveRightConfig);

		config["left"].set<picojson::object>(gloveLeftConfig);
		config["right"].set<picojson::object>(gloveRightConfig);

		// Write do automatic launch
		config["doAutoLaunch"].set<bool>(state.doAutoLaunch);

		picojson::value rootV;
		rootV.set<picojson::object>(config);

		fileStream << rootV.serialize(true);

		fileStream.close();
	}
}