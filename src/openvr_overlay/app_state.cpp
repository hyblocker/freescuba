#include "app_state.hpp"

AppState::AppState() {

    doAutoLaunch                                        = true;
    dongleAvailable                                     = false;
    gloveLeft                                           = {};
    gloveRight                                          = {};
    uiState                                             = {};
    ipcClient                                           = nullptr;

    uiState.leftGloveBatteryBuffer.Init(BATTERY_WINDOW_SIZE);
    uiState.rightGloveBatteryBuffer.Init(BATTERY_WINDOW_SIZE);

    // Joystick calibration must initially set mins to max value
    gloveLeft.calibration.joystick.XMax                 = 62000;
    gloveLeft.calibration.joystick.XMin                 = 18000;
    gloveLeft.calibration.joystick.YMax                 = 55000;
    gloveLeft.calibration.joystick.YMin                 = 8000;
    gloveLeft.calibration.joystick.forwardAngle         = -0.20632386207580566;
    
    // Default calibration for right glove joystick
    gloveRight.calibration.joystick.XMax                = 62000;
    gloveRight.calibration.joystick.XMin                = 14000;
    gloveRight.calibration.joystick.YMax                = 59000;
    gloveRight.calibration.joystick.YMin                = 11000;
    gloveRight.calibration.joystick.forwardAngle        = -3.0471484661102295f;

    // Default deadzone
    gloveLeft.calibration.joystick.threshold            = 0.1f;
    gloveRight.calibration.joystick.threshold           = 0.1f;

    // Default finger calibration
    gloveLeft.calibration.fingers.thumbRoot.close       = 0xFFFF;
    gloveLeft.calibration.fingers.thumbTip.close        = 0xFFFF;
    gloveLeft.calibration.fingers.indexRoot.close       = 0xFFFF;
    gloveLeft.calibration.fingers.indexTip.close        = 0xFFFF;
    gloveLeft.calibration.fingers.middleRoot.close      = 0xFFFF;
    gloveLeft.calibration.fingers.middleTip.close       = 0xFFFF;
    gloveLeft.calibration.fingers.ringRoot.close        = 0xFFFF;
    gloveLeft.calibration.fingers.ringTip.close         = 0xFFFF;
    gloveLeft.calibration.fingers.pinkyRoot.close       = 0xFFFF;
    gloveLeft.calibration.fingers.pinkyTip.close        = 0xFFFF;

    // Default pose calibration for left glove
    gloveLeft.calibration.poseOffset.pos.v[0]           =  0.022108916431138825;
    gloveLeft.calibration.poseOffset.pos.v[1]           = -0.10298597531413284;
    gloveLeft.calibration.poseOffset.pos.v[2]           = -0.043071794351218051;

    gloveLeft.calibration.poseOffset.rot.w              =  0.79839363620734938;
    gloveLeft.calibration.poseOffset.rot.x              =  0.56994138349383228;
    gloveLeft.calibration.poseOffset.rot.y              = -0.0095891559420182571;
    gloveLeft.calibration.poseOffset.rot.z              =  0.1940166723069508;

    // Default pose calibration for right glove
    gloveRight.calibration.poseOffset.pos.v[0]          =  0.014676248807481751;
    gloveRight.calibration.poseOffset.pos.v[1]          =  0.12989163327871586;
    gloveRight.calibration.poseOffset.pos.v[2]          = -0.07395779910121722;

    gloveRight.calibration.poseOffset.rot.w             =  0.74633244094972939;
    gloveRight.calibration.poseOffset.rot.x             = -0.61839448536096064;
    gloveRight.calibration.poseOffset.rot.y             =  0.15040583272822428;
    gloveRight.calibration.poseOffset.rot.z             = -0.19481846304316558;

    // Default gesture values for left glove
    gloveLeft.calibration.gestures.grip.activate        = 0.508f;
    gloveLeft.calibration.gestures.grip.deactivate      = 0.644f;
    gloveLeft.calibration.gestures.thumb.activate       = 0.757f;
    gloveLeft.calibration.gestures.thumb.deactivate     = 0.757f;
    gloveLeft.calibration.gestures.trigger.activate     = 0.850f;
    gloveLeft.calibration.gestures.trigger.deactivate   = 0.722f;

    // Default gesture values for right glove
    gloveRight.calibration.gestures.grip.activate       = 0.551f;
    gloveRight.calibration.gestures.grip.deactivate     = 0.683f;
    gloveRight.calibration.gestures.thumb.activate      = 0.757f;
    gloveRight.calibration.gestures.thumb.deactivate    = 0.757f;
    gloveRight.calibration.gestures.trigger.activate    = 0.850f;
    gloveRight.calibration.gestures.trigger.deactivate  = 0.722f;

    // Default battery life
    gloveLeft.gloveBattery                              = protocol::GLOVE_BATTERY_INVALID;
    gloveLeft.gloveBatteryRaw                           = protocol::GLOVE_BATTERY_INVALID;
    gloveRight.gloveBattery                             = protocol::GLOVE_BATTERY_INVALID;
    gloveRight.gloveBatteryRaw                          = protocol::GLOVE_BATTERY_INVALID;
}