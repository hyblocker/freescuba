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
        const std::function<void(const ContactGloveDevice_t, const GloveInputData_t&)> inputCallback,
        const std::function<void(const ContactGloveDevice_t, const GlovePacketFingers_t&)> fingersCallback,
        const std::function<void(const DevicesStatus_t&)> statusCallback,
        const std::function<void(const DevicesFirmware_t&)> firmwareCallback);
    bool IsConnected() const;
    void Disconnect();
    void WriteCommand(const std::string& command);

private:
    bool Connect();
    bool SetCommunicationTimeout(
        const uint32_t readIntervalTimeout,
        const uint32_t readTotalTimeoutMultiplier,
        const uint32_t readTotalTimeoutConstant,
        const uint32_t writeTotalTimeoutMultiplier,
        const uint32_t WriteTotalTimeoutConstant) const;
    void ListenerThread();
    bool ReceiveNextPacket(std::string& buff);
    bool PurgeBuffer() const;
    void WaitAttemptConnection();
    bool DisconnectFromDevice(bool writeDeactivate = true);
    bool WriteQueued();
    int GetComPort() const;

    bool DecodePacket(const uint8_t* pData, const size_t length, ContactGlovePacket_t* outPacket) const;

    void LogMessage(const char* message) const;
    void LogError(const char* message) const;
    void LogWarning(const char* message) const;

private:
    std::atomic<bool> m_isConnected;
    // Serial com handler
    HANDLE m_hSerial;
    // Error tracking
    DWORD m_errors;

    std::string m_port;

    std::atomic<bool> m_threadActive;
    std::thread m_serialThread;

    std::mutex m_writeMutex;

    std::string m_queuedWrite;

    // Callbacks
    std::function<void(const ContactGloveDevice_t handedness, const GlovePacketFingers_t&)> m_fingersCallback;
    std::function<void(const ContactGloveDevice_t handedness, const GloveInputData_t&)> m_inputCallback;
    std::function<void(const DevicesStatus_t&)> m_statusCallback;
    std::function<void(const DevicesFirmware_t&)> m_firmwareCallback;
};
