#include "serial_communication.hpp"
#include <SetupAPI.h>

#include <chrono>
#include "cobs.hpp"
#include <iomanip>

static const std::string c_serialDeviceId = "VID_10C4&PID_7B27";
static const uint32_t LISTENER_WAIT_TIME = 1000;

static void PrintBuffer(const std::string name, const uint8_t* buffer, const size_t size) {
    // @FIXME: Can we make this neater using printf?
    std::cout << "uint8_t " << name << "[" << std::dec << size << "] = { " << std::flush;

    for (size_t i = 0; i < size; i++) {
        std::cout << "0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << (((int)buffer[i]) & 0xFF) << std::flush;
        if (i != size - 1) {
            std::cout << ", " << std::flush;
        }
    }

    std::cout << " };" << std::endl;
}

int SerialCommunicationManager::GetComPort() const {

    HDEVINFO DeviceInfoSet;
    SP_DEVINFO_DATA DeviceInfoData;
    DEVPROPTYPE ulPropertyType;
    DWORD DeviceIndex       = 0;
    std::string DevEnum     = "USB";
    char szBuffer[1024]     = { 0 };
    DWORD dwSize            = 0;
    DWORD Error             = 0;

    DeviceInfoSet = SetupDiGetClassDevsA(NULL, DevEnum.c_str(), NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT);

    if (DeviceInfoSet == INVALID_HANDLE_VALUE) {
        return -1;
    }

    ZeroMemory(&DeviceInfoData, sizeof(SP_DEVINFO_DATA));
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    // Receive information about an enumerated device

    while ( SetupDiEnumDeviceInfo(DeviceInfoSet, DeviceIndex, &DeviceInfoData) ) {
        DeviceIndex++;

        // Retrieves a specified Plug and Play device property
        if (SetupDiGetDeviceRegistryPropertyA(
            DeviceInfoSet,
            &DeviceInfoData,
            SPDRP_HARDWAREID,
            &ulPropertyType,
            (BYTE*)szBuffer,
            sizeof(szBuffer),  // The size, in bytes
            &dwSize)) {

            if ( std::string(szBuffer).find(c_serialDeviceId) == std::string::npos ) {
                continue;
            }

            HKEY hDeviceRegistryKey = { 0 };
            hDeviceRegistryKey      = SetupDiOpenDevRegKey(DeviceInfoSet, &DeviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);

            if ( hDeviceRegistryKey == INVALID_HANDLE_VALUE ) {
                Error = GetLastError();
                break;
            } else {
                char pszPortName[20]    = { 0 };
                DWORD dwSize            = sizeof(pszPortName);
                DWORD dwType            = 0;

                if (
                    ( RegQueryValueExA(hDeviceRegistryKey, "PortName", NULL, &dwType, (LPBYTE)pszPortName, &dwSize) == ERROR_SUCCESS ) &&
                    ( dwType == REG_SZ ) )
                {
                    // @FIXME: Avoid allocating memory by using a std::string
                    //         Should we consider implementing a substr function which takes in a char* ?
                    std::string sPortName = pszPortName;
                    try {
                        if ( sPortName.substr( 0, 3 ) == "COM" ) {
                            int nPortNr = std::stoi( pszPortName + 3 );
                            if ( nPortNr != 0 ) {
                                return nPortNr;
                            }
                        }
                    } catch ( ... ) {
                        printf("Parsing failed for a port\n");
                    }
                }
                RegCloseKey(hDeviceRegistryKey);
            }
        }
    } // while ( SetupDiEnumDeviceInfo(DeviceInfoSet, DeviceIndex, &DeviceInfoData) )

    if (DeviceInfoSet) {
        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
    }

    return -1;
}

static std::string GetLastErrorAsString() {
    DWORD errorMessageID = ::GetLastError();
    if (errorMessageID == 0) {
        return std::string();
    }

    LPSTR messageBuffer = nullptr;

    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        errorMessageID,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&messageBuffer,
        0,
        NULL);

    std::string message(messageBuffer, size);

    LocalFree(messageBuffer);

    return message;
}

bool SerialCommunicationManager::SetCommunicationTimeout(
    const uint32_t ReadIntervalTimeout,
    const uint32_t ReadTotalTimeoutMultiplier,
    const uint32_t ReadTotalTimeoutConstant,
    const uint32_t WriteTotalTimeoutMultiplier,
    const uint32_t WriteTotalTimeoutConstant) const {

    COMMTIMEOUTS timeout = {
        .ReadIntervalTimeout            = MAXDWORD,
        .ReadTotalTimeoutMultiplier     = MAXDWORD,
        .ReadTotalTimeoutConstant       = 10,
        .WriteTotalTimeoutMultiplier    = WriteTotalTimeoutMultiplier,
        .WriteTotalTimeoutConstant      = WriteTotalTimeoutConstant,
    };

    if (!SetCommTimeouts(m_hSerial, &timeout)) {
        return false;
    }

    return true;
}

bool SerialCommunicationManager::Connect() {
    // We're not yet connected
    m_isConnected = false;

    LogMessage("Attempting connection to dongle...");

    short port = GetComPort();
    m_port = "\\\\.\\COM" + std::to_string(port);

    // Try to connect to the given port throuh CreateFile
    m_hSerial = CreateFileA(m_port.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (m_hSerial == INVALID_HANDLE_VALUE) {
        LogError("Received error connecting to port");
        return false;
    }

    // If connected we try to set the comm parameters
    DCB dcbSerialParams = { 0 };

    if (!GetCommState(m_hSerial, &dcbSerialParams)) {
        LogError("Failed to get current port parameters");
        return false;
    }

    // Define serial connection parameters for the arduino board
    dcbSerialParams.BaudRate        = CBR_115200;
    dcbSerialParams.ByteSize        = 8;
    dcbSerialParams.StopBits        = ONESTOPBIT;
    dcbSerialParams.Parity          = NOPARITY;

    // reset upon establishing a connection
    dcbSerialParams.fDtrControl     = DTR_CONTROL_ENABLE;
    dcbSerialParams.XonChar         = 0x11;
    dcbSerialParams.XoffLim         = 0x4000;
    dcbSerialParams.XoffChar        = 0x13;
    dcbSerialParams.EofChar         = 0x1A;
    dcbSerialParams.EvtChar         = 0;

    // set the parameters and check for their proper application
    if (!SetCommState(m_hSerial, &dcbSerialParams)) {
        LogError("Failed to set serial parameters");
        return false;
    }

    if (!SetCommunicationTimeout(MAXDWORD, MAXDWORD, 1000, 5, 0)) {
        LogError("Failed to set communication timeout");
        return false;
    }

    COMMTIMEOUTS timeout = {
        .ReadIntervalTimeout            = MAXDWORD,
        .ReadTotalTimeoutMultiplier     = MAXDWORD,
        .ReadTotalTimeoutConstant       = 10,
        .WriteTotalTimeoutMultiplier    = 5,
        .WriteTotalTimeoutConstant      = 0,
    };

    if (!SetCommTimeouts(m_hSerial, &timeout)) {
        LogError("Failed to set comm timeouts");

        return false;
    }

    // If everything went fine we're connected
    m_isConnected = true;

    LogMessage("Successfully connected to dongle");
    return true;
};

void SerialCommunicationManager::WaitAttemptConnection() {
    LogMessage("Attempting to connect to dongle");
    while (m_threadActive && !IsConnected() && !Connect()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(LISTENER_WAIT_TIME));
    }
    if (!m_threadActive) {
        return;
    }
}

void SerialCommunicationManager::BeginListener(
    const std::function<void(const ContactGloveDevice_t handedness, const GloveInputData_t&)> inputCallback,
    const std::function<void(const ContactGloveDevice_t handedness, const GlovePacketFingers_t&)> fingersCallback,
    const std::function<void(const DevicesStatus_t&)> statusCallback,
    const std::function<void(const DevicesFirmware_t&)> firmwareCallback) {

    m_fingersCallback   = fingersCallback;
    m_inputCallback     = inputCallback;
    m_statusCallback    = statusCallback;
    m_firmwareCallback  = firmwareCallback;

    m_threadActive = true;
    m_serialThread = std::thread(&SerialCommunicationManager::ListenerThread, this);
}

void SerialCommunicationManager::ListenerThread() {
    WaitAttemptConnection();

    PurgeBuffer();

    while (m_threadActive) {
        std::string receivedString;

        try {
            if (!ReceiveNextPacket(receivedString)) {
                LogMessage("Detected device error. Disconnecting device and attempting reconnection...");
                // UpdateDongleState(VRDongleState::disconnected);

                if (DisconnectFromDevice(false)) {
                    WaitAttemptConnection();
                    LogMessage("Successfully reconnected to device");
                    continue;
                }

                LogMessage("Could not disconnect from device. Closing listener...");
                Disconnect();

                return;
            }
        }
        catch (const std::invalid_argument& ia) {
            LogMessage((std::string("Received error from encoding: ") + ia.what()).c_str());
        }
        catch (...) {
            LogMessage("Received unknown error attempting to decode packet.");
        }

        // If we haven't received anything don't bother with trying to decode anything
        if (receivedString.empty()) {
            goto finish;
        }

    finish:
        // write anything we need to
        WriteQueued();
    }
}

bool SerialCommunicationManager::ReceiveNextPacket(std::string& buff) {
    DWORD dwRead    = 0;

    char thisChar   = 0x00;
    char lastChar   = 0x00;

    do {
        lastChar = thisChar;

        if (!ReadFile(m_hSerial, &thisChar, 1, &dwRead, NULL)) {
            LogError("Error reading from file");
            return false;
        }

        if (dwRead == 0) {
            LogWarning("No packet received within timeout\n");
            break;
        }

        // Delimeter is 0, since data is encoded using COBS
        if (thisChar == 0) {
            // the last byte will be 0
            uint8_t* pData = (uint8_t*)malloc(buff.size());
            if (pData != 0) {
                memset(pData, 0, buff.size());
                memcpy(pData, buff.c_str(), buff.size());

                // Decode data
                cobs::decode(pData, buff.size());

                crc CRC_Result = F_CRC_CalculateCheckSum(pData, buff.size());

                if (CRC_Result == CRC_RESULT_OK) {
                    ContactGlovePacket_t packet = {};
                    if (DecodePacket(pData, buff.size(), &packet)) {
                        switch (packet.type) {
                            // Invoke callback
                        case PacketType_t::GloveLeftData:
                            m_inputCallback(ContactGloveDevice_t::LeftGlove, packet.packet.gloveData);
                            break;
                        case PacketType_t::GloveRightData:
                            m_inputCallback(ContactGloveDevice_t::RightGlove, packet.packet.gloveData);
                            break;
                        case PacketType_t::GloveLeftFingers:
                            m_fingersCallback(ContactGloveDevice_t::LeftGlove, packet.packet.gloveFingers);
                            break;
                        case PacketType_t::GloveRightFingers:
                            m_fingersCallback(ContactGloveDevice_t::RightGlove, packet.packet.gloveFingers);
                            break;
                        case PacketType_t::DevicesStatus:
                            m_statusCallback(packet.packet.status);
                            break;
                        case PacketType_t::DevicesFirmware:
                            m_firmwareCallback(packet.packet.firmware);
                            break;
                        }
                    }
                }

                free(pData);
            }

            // Clear buffer
            buff = "";
        }
        else {
            // Only add the current character if 0 we aren't on the delimeter
            buff += thisChar;
        }

        if (buff.size() > MAX_PACKET_SIZE) {
            LogError("Overflowed controller input. Resetting...");
            buff = "";
            break;
        }

    } while ((!(thisChar == -1 && lastChar == -1)) && m_threadActive);

    return true;
}

void SerialCommunicationManager::WriteCommand(const std::string& command) {
    std::scoped_lock lock(m_writeMutex);

    if (!m_isConnected) {
        LogMessage("Cannot write to dongle as it is not connected.");

        return;
    }

    m_queuedWrite = command + "\r\n";
}

bool SerialCommunicationManager::WriteQueued() {
    std::scoped_lock lock(m_writeMutex);

    if (!m_isConnected) {
        return false;
    }

    if (m_queuedWrite.empty()) {
        return true;
    }

    const char* buf = m_queuedWrite.c_str();
    DWORD bytesSend;
    if (!WriteFile(this->m_hSerial, (void*)buf, (DWORD)m_queuedWrite.size(), &bytesSend, 0)) {
        LogError("Error writing to port");
        return false;
    }

    printf("Wrote: %s\n", m_queuedWrite.c_str());

    m_queuedWrite.clear();

    return true;
}

bool SerialCommunicationManager::PurgeBuffer() const {
    return PurgeComm(m_hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);
}

void SerialCommunicationManager::Disconnect() {
    printf("Attempting to disconnect serial\n");
    if (m_threadActive.exchange(false)) {
        CancelIoEx(m_hSerial, nullptr);
        m_serialThread.join();

        printf("Serial joined\n");
    }

    if (IsConnected()) {
        DisconnectFromDevice(true);
    }

    printf("Serial finished disconnecting\n");
}

bool SerialCommunicationManager::DisconnectFromDevice(bool writeDeactivate) {
    if (writeDeactivate) {
        WriteCommand("BP+VS");
    }
    else {
        LogMessage("Not deactivating Input API as dongle was forcibly disconnected");
    }

    if (!CloseHandle(m_hSerial)) {
        LogError("Error disconnecting from device");
        return false;
    }

    m_isConnected = false;

    LogMessage("Successfully disconnected from device");
    return true;
};

bool SerialCommunicationManager::IsConnected() const {
    return m_isConnected;
};

void SerialCommunicationManager::LogError(const char* message) const {
    // message with port name and last error
    printf("%s (%s) - Error: %s\n", message, m_port.c_str(), GetLastErrorAsString().c_str());
}

void SerialCommunicationManager::LogWarning(const char* message) const {
    // message with port name
    printf("%s (%s) - Warning: %s\n", message, m_port.c_str(), GetLastErrorAsString().c_str());
}

void SerialCommunicationManager::LogMessage(const char* message) const {
    // message with port name
    printf("%s (%s)\n", message, m_port.c_str());
}

bool SerialCommunicationManager::DecodePacket(const uint8_t* pData, const size_t length, ContactGlovePacket_t* outPacket) const {

    bool decoded = false;

    switch (pData[0]) {
    case GLOVE_LEFT_PACKET_DATA:
    {
        // Assign type
        outPacket->type = PacketType_t::GloveLeftData;

        // Extract data
        outPacket->packet.gloveData.joystickX = ((uint16_t*)(pData))[1];
        outPacket->packet.gloveData.joystickY = ((uint16_t*)(pData))[2];

        uint8_t button_mask = ((uint8_t*)pData)[1];

        // KNOWN VALUES:
        // None             :: 0x3F   0011 1111
        // System Up        :: 0x2F   0010 1111
        // System Down      :: 0x37   0011 0111
        // A (BTN_DOWN)     :: 0x3E   0011 1110
        // B (BTN_UP)       :: 0x3D   0011 1101
        // Joystick Click   :: 0x3B   0011 1011
        // Magnetra unavail :: 0x1F   0001 1111

        outPacket->packet.gloveData.hasMagnetra     =   (button_mask & CONTACT_GLOVE_INPUT_MASK_MAGNETRA_PRESENT)   == CONTACT_GLOVE_INPUT_MASK_MAGNETRA_PRESENT;
        outPacket->packet.gloveData.systemUp        = !((button_mask & CONTACT_GLOVE_INPUT_MASK_SYSTEM_UP)          == CONTACT_GLOVE_INPUT_MASK_SYSTEM_UP);
        outPacket->packet.gloveData.systemDown      = !((button_mask & CONTACT_GLOVE_INPUT_MASK_SYSTEM_DOWN)        == CONTACT_GLOVE_INPUT_MASK_SYSTEM_DOWN);
        outPacket->packet.gloveData.buttonUp        = !((button_mask & CONTACT_GLOVE_INPUT_MASK_BUTTON_UP)          == CONTACT_GLOVE_INPUT_MASK_BUTTON_UP);
        outPacket->packet.gloveData.buttonDown      = !((button_mask & CONTACT_GLOVE_INPUT_MASK_BUTTON_DOWN)        == CONTACT_GLOVE_INPUT_MASK_BUTTON_DOWN);
        outPacket->packet.gloveData.joystickClick   = !((button_mask & CONTACT_GLOVE_INPUT_MASK_JOYSTICK_CLICK)     == CONTACT_GLOVE_INPUT_MASK_JOYSTICK_CLICK);

        // printf("data[L]:: magnetra:%d, sysUp:%d, sysDn:%d, btnUp:%d, btnDn:%d, joyClk:%d (0x%04hX,0x%04hX)\n",
        //     outPacket->packet.gloveData.hasMagnetra,
        //     outPacket->packet.gloveData.systemUp,
        //     outPacket->packet.gloveData.systemDown,
        //     outPacket->packet.gloveData.buttonUp,
        //     outPacket->packet.gloveData.buttonDown,
        //     outPacket->packet.gloveData.joystickClick,
        //     outPacket->packet.gloveData.joystickX,
        //     outPacket->packet.gloveData.joystickY);

        // Left glove data packet
        // PrintBuffer("glove_data_left", pData, length);

        decoded = true;
    }

    break;
    case GLOVE_RIGHT_PACKET_DATA:
    {
        // Assign type
        outPacket->type = PacketType_t::GloveRightData;

        // Extract data
        outPacket->packet.gloveData.joystickX = ((uint16_t*)(pData))[1];
        outPacket->packet.gloveData.joystickY = ((uint16_t*)(pData))[2];

        uint8_t button_mask = ((uint8_t*)pData)[1];

        // KNOWN VALUES:
        // None             :: 0x3F   0011 1111
        // System Up        :: 0x2F   0010 1111
        // System Down      :: 0x37   0011 0111
        // A (BTN_DOWN)     :: 0x3E   0011 1110
        // B (BTN_UP)       :: 0x3D   0011 1101
        // Joystick Click   :: 0x3B   0011 1011
        // Magnetra unavail :: 0x1F   0001 1111

        outPacket->packet.gloveData.hasMagnetra     =   (button_mask & CONTACT_GLOVE_INPUT_MASK_MAGNETRA_PRESENT)   == CONTACT_GLOVE_INPUT_MASK_MAGNETRA_PRESENT;
        outPacket->packet.gloveData.systemUp        = !((button_mask & CONTACT_GLOVE_INPUT_MASK_SYSTEM_UP)          == CONTACT_GLOVE_INPUT_MASK_SYSTEM_UP);
        outPacket->packet.gloveData.systemDown      = !((button_mask & CONTACT_GLOVE_INPUT_MASK_SYSTEM_DOWN)        == CONTACT_GLOVE_INPUT_MASK_SYSTEM_DOWN);
        outPacket->packet.gloveData.buttonUp        = !((button_mask & CONTACT_GLOVE_INPUT_MASK_BUTTON_UP)          == CONTACT_GLOVE_INPUT_MASK_BUTTON_UP);
        outPacket->packet.gloveData.buttonDown      = !((button_mask & CONTACT_GLOVE_INPUT_MASK_BUTTON_DOWN)        == CONTACT_GLOVE_INPUT_MASK_BUTTON_DOWN);
        outPacket->packet.gloveData.joystickClick   = !((button_mask & CONTACT_GLOVE_INPUT_MASK_JOYSTICK_CLICK)     == CONTACT_GLOVE_INPUT_MASK_JOYSTICK_CLICK);

        // printf("data[R]:: magnetra:%d, sysUp:%d, sysDn:%d, btnUp:%d, btnDn:%d, joyClk:%d (0x%04hX,0x%04hX)\n",
        //     outPacket->packet.gloveData.hasMagnetra,
        //     outPacket->packet.gloveData.systemUp,
        //     outPacket->packet.gloveData.systemDown,
        //     outPacket->packet.gloveData.buttonUp,
        //     outPacket->packet.gloveData.buttonDown,
        //     outPacket->packet.gloveData.joystickClick,
        //     outPacket->packet.gloveData.joystickX,
        //     outPacket->packet.gloveData.joystickY);

        // Right glove data packet
        // PrintBuffer("glove_data_right", pData, length);

        decoded = true;
    }
    break;
    case DEVICES_VERSIONS:
    {
        // Assign type
        outPacket->type = PacketType_t::DevicesFirmware;

        // data is 0x01 0x06 3 times

        // This is version a.b
        outPacket->packet.firmware.dongleMajor      = ((uint8_t*)pData)[1];
        outPacket->packet.firmware.dongleMinor      = ((uint8_t*)pData)[2];
        outPacket->packet.firmware.gloveLeftMajor   = ((uint8_t*)pData)[3];
        outPacket->packet.firmware.gloveLeftMinor   = ((uint8_t*)pData)[4];
        outPacket->packet.firmware.gloveRightMajor  = ((uint8_t*)pData)[5];
        outPacket->packet.firmware.gloveRightMinor  = ((uint8_t*)pData)[6];

        // PrintBuffer("firmware_version", pData, length);

        decoded = true;
    }
    break;

    case DEVICES_STATUS:
    {
        // Assign type
        outPacket->type = PacketType_t::DevicesStatus;

        // uint8_t dongle4[8] = { 0x00, 0x1E, 0x50, 0x5F, 0x5A, 0x60, 0x01, 0xA2 };

        outPacket->packet.status.gloveLeftBattery   = ((uint8_t*)pData)[1];
        outPacket->packet.status.gloveRightBattery  = ((uint8_t*)pData)[3];

        // printf("glove[L]:: %d%%  ; glove[R]:: %d%%\n", outPacket->packet.status.gloveLeftBattery, outPacket->packet.status.gloveRightBattery);

        // PrintBuffer("dongle4", pData, length);

        decoded = true;
    }
    break;


    case GLOVE_POWER_ON_PACKET:
    {
        // Assign type
        outPacket->type = PacketType_t::GlovePowerOn;

        // PrintBuffer("glove_power_on", pData, length);

        decoded = true;
    }
    break;


    case GLOVE_LEFT_PACKET_FINGERS:
    {
        // Assign type
        outPacket->type = PacketType_t::GloveLeftFingers;
        // Extract data
        outPacket->packet.gloveFingers.fingerThumbTip   = ((uint16_t*)(pData + 1))[9];
        outPacket->packet.gloveFingers.fingerThumbRoot  = ((uint16_t*)(pData + 1))[8];
        outPacket->packet.gloveFingers.fingerIndexTip   = ((uint16_t*)(pData + 1))[7];
        outPacket->packet.gloveFingers.fingerIndexRoot  = ((uint16_t*)(pData + 1))[6];
        outPacket->packet.gloveFingers.fingerMiddleTip  = ((uint16_t*)(pData + 1))[5];
        outPacket->packet.gloveFingers.fingerMiddleRoot = ((uint16_t*)(pData + 1))[4];
        outPacket->packet.gloveFingers.fingerRingTip    = ((uint16_t*)(pData + 1))[3];
        outPacket->packet.gloveFingers.fingerRingRoot   = ((uint16_t*)(pData + 1))[2];
        outPacket->packet.gloveFingers.fingerPinkyTip   = ((uint16_t*)(pData + 1))[1];
        outPacket->packet.gloveFingers.fingerPinkyRoot  = ((uint16_t*)(pData + 1))[0];

        // printf(
        //     "fingers[L]:: (%d, %d) (%d, %d) (%d, %d) (%d, %d) (%d, %d)\n",
        //     outPacket->packet.gloveFingers.fingerThumbTip,
        //     outPacket->packet.gloveFingers.fingerThumbRoot,
        //     outPacket->packet.gloveFingers.fingerIndexTip,
        //     outPacket->packet.gloveFingers.fingerIndexRoot,
        //     outPacket->packet.gloveFingers.fingerMiddleTip,
        //     outPacket->packet.gloveFingers.fingerMiddleRoot,
        //     outPacket->packet.gloveFingers.fingerRingTip,
        //     outPacket->packet.gloveFingers.fingerRingRoot,
        //     outPacket->packet.gloveFingers.fingerPinkyTip,
        //     outPacket->packet.gloveFingers.fingerPinkyRoot);

        // PrintBuffer("glove_left_fingers", pData, length);

        decoded = true;

    }
    break;
    case GLOVE_LEFT_PACKET_IMU:
    {
        // @TODO: Imu data is not usable

        // Assign type
        outPacket->type = PacketType_t::GloveLeftImu;
        // Extract data
        outPacket->packet.gloveImu.imu1 = ((uint16_t*)(pData + 1))[0];
        outPacket->packet.gloveImu.imu2 = ((uint16_t*)(pData + 1))[1];

        outPacket->packet.gloveImu.imu3 = ((uint16_t*)(pData + 1))[2];
        outPacket->packet.gloveImu.imu4 = ((uint16_t*)(pData + 1))[3];

        outPacket->packet.gloveImu.imu5 = ((float*)(pData + 1))[4];
        outPacket->packet.gloveImu.imu6 = ((float*)(pData + 1))[5];

        // printf(
        //     "imu[L]:: %d, %d, %d, %d [%.6f, %.6f]\n",
        //     outPacket->packet.gloveImu.imu1,
        //     outPacket->packet.gloveImu.imu2,
        //     outPacket->packet.gloveImu.imu3,
        //     outPacket->packet.gloveImu.imu4,
        //     outPacket->packet.gloveImu.imu5,
        //     outPacket->packet.gloveImu.imu6);

        // PrintBuffer("glove_left_small", pData, length);
        decoded = true;

    }
    break;

    case GLOVE_RIGHT_PACKET_FINGERS:
    {
        // Assign type
        outPacket->type = PacketType_t::GloveRightFingers;

        // Extract data
        outPacket->packet.gloveFingers.fingerThumbTip   = ((uint16_t*)(pData + 1))[9];
        outPacket->packet.gloveFingers.fingerThumbRoot  = ((uint16_t*)(pData + 1))[8];
        outPacket->packet.gloveFingers.fingerIndexTip   = ((uint16_t*)(pData + 1))[7];
        outPacket->packet.gloveFingers.fingerIndexRoot  = ((uint16_t*)(pData + 1))[6];
        outPacket->packet.gloveFingers.fingerMiddleTip  = ((uint16_t*)(pData + 1))[5];
        outPacket->packet.gloveFingers.fingerMiddleRoot = ((uint16_t*)(pData + 1))[4];
        outPacket->packet.gloveFingers.fingerRingTip    = ((uint16_t*)(pData + 1))[3];
        outPacket->packet.gloveFingers.fingerRingRoot   = ((uint16_t*)(pData + 1))[2];
        outPacket->packet.gloveFingers.fingerPinkyTip   = ((uint16_t*)(pData + 1))[1];
        outPacket->packet.gloveFingers.fingerPinkyRoot  = ((uint16_t*)(pData + 1))[0];

        // printf(
        //     "fingers[R]:: (%d, %d) (%d, %d) (%d, %d) (%d, %d) (%d, %d)\n",
        //     outPacket->packet.gloveFingers.fingerThumbTip,
        //     outPacket->packet.gloveFingers.fingerThumbRoot,
        //     outPacket->packet.gloveFingers.fingerIndexTip,
        //     outPacket->packet.gloveFingers.fingerIndexRoot,
        //     outPacket->packet.gloveFingers.fingerMiddleTip,
        //     outPacket->packet.gloveFingers.fingerMiddleRoot,
        //     outPacket->packet.gloveFingers.fingerRingTip,
        //     outPacket->packet.gloveFingers.fingerRingRoot,
        //     outPacket->packet.gloveFingers.fingerPinkyTip,
        //     outPacket->packet.gloveFingers.fingerPinkyRoot);

        // PrintBuffer("glove_right_fingers", pData, length);

        decoded = true;

    }
    break;
    case GLOVE_RIGHT_PACKET_IMU:
    {
        // @TODO: Imu data is not usable

        // Assign type
        outPacket->type = PacketType_t::GloveRightImu;
        // Extract data
        outPacket->packet.gloveImu.imu1 = ((uint16_t*)(pData + 1))[0];
        outPacket->packet.gloveImu.imu2 = ((uint16_t*)(pData + 1))[1];

        outPacket->packet.gloveImu.imu3 = ((uint16_t*)(pData + 1))[2];
        outPacket->packet.gloveImu.imu4 = ((uint16_t*)(pData + 1))[3];

        outPacket->packet.gloveImu.imu5 = ((float*)(pData + 1))[0];
        outPacket->packet.gloveImu.imu6 = ((float*)(pData + 1))[1];

        // printf(
        //     "imu[R]:: %d, %d, %d, %d [%.6f, %.6f]\n",
        //     outPacket->packet.gloveImu.imu1,
        //     outPacket->packet.gloveImu.imu2,
        //     outPacket->packet.gloveImu.imu3,
        //     outPacket->packet.gloveImu.imu4,
        //     outPacket->packet.gloveImu.imu5,
        //     outPacket->packet.gloveImu.imu6);

        // PrintBuffer("glove_right_small", pData, length);
        decoded = true;

    }

    break;

    default:
        // @FIXME: Use proper logging library
        printf("[WARN] Got unknown packet with command code 0x%02hX!!\n", pData[1]);
        PrintBuffer("unknown_packet", pData, length);
        break;
    }

    return decoded;
}

/*
WRITE FILE:
```
02 01 01 02 f0 01 03 f0 02 00
05 18 01 01 e1 00
05 18 02 01 de 00
05 18 01 0b d7 00
05 18 02 0b e8 00
05 16 01 04 03 01 03 ff 70 00
05 16 02 04 03 01 03 ff 0b 00
03 0f 32 02 64 03 ff 9f 00
```
*/