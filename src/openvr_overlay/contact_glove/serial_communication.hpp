#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <functional>
#include <memory>
#include <atomic>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <thread>

#include "crc.hpp"
#include "contact_glove_structs.hpp"

class SerialCommunicationManager {
public:
    SerialCommunicationManager()
        : m_isConnected(false), m_hSerial(0), m_errors(0), m_writeMutex(), m_queuedWrite("") {};

    void BeginListener(
        const std::function<void(const ContactGloveDevice, const GloveInputData_t&)> inputCallback,
        const std::function<void(const ContactGloveDevice, const GlovePacketFingers_t&)> fingersCallback,
        const std::function<void(const DevicesStatus_t&)> statusCallback,
        const std::function<void(const DevicesFirmware_t&)> firmwareCallback);
    bool IsConnected();
    void Disconnect();
    void WriteCommand(const std::string& command);

private:
    bool Connect();
    bool SetCommunicationTimeout(
        unsigned long readIntervalTimeout,
        unsigned long readTotalTimeoutMultiplier,
        unsigned long readTotalTimeoutConstant,
        unsigned long writeTotalTimeoutMultiplier,
        unsigned long WriteTotalTimeoutConstant);
    void ListenerThread();
    bool ReceiveNextPacket(std::string& buff);
    bool PurgeBuffer();
    void WaitAttemptConnection();
    bool DisconnectFromDevice(bool writeDeactivate = true);
    bool WriteQueued();
    int GetComPort();

    void LogMessage(const char* message);
    void LogError(const char* message);

    std::atomic<bool> m_isConnected;
    // Serial comm handler
    HANDLE m_hSerial;
    // Error tracking
    DWORD m_errors;

    std::string m_port;

    std::atomic<bool> m_threadActive;
    std::thread m_serialThread;

    std::mutex m_writeMutex;

    std::string m_queuedWrite;

    std::function<void(const ContactGloveDevice handedness, const GlovePacketFingers_t&)> m_fingersCallback;
    std::function<void(const ContactGloveDevice handedness, const GloveInputData_t&)> m_inputCallback;
    std::function<void(const DevicesStatus_t&)> m_statusCallback;
    std::function<void(const DevicesFirmware_t&)> m_firmwareCallback;
};
