#include "usb/device/switch_device.h"
#include "protocols/ps4.hpp"
#include "enums.pb.h"
#include "hid_reports.h"
#include "config.hpp"
#include "usb/usb_devices.h"
#include "pico/rand.h"

uint8_t const desc_hid_report_switch_pro[] =
    {
        TUD_HID_REPORT_SWITCH()};
SwitchGamepadDevice::SwitchGamepadDevice()
{
    playerID = 0;
    last_report_counter = 0;
    handshakeCounter = 0;
    isReady = false;

    deviceInfo = {
        .majorVersion = 0x04,
        .minorVersion = 0x91,
        .controllerType = SwitchControllerType::SWITCH_TYPE_PRO_CONTROLLER,
        .unknown00 = 0x02,
        // MAC address in reverse
        .macAddress = {0x7c, 0xbb, 0x8a, (uint8_t)(get_rand_32() % 0xff), (uint8_t)(get_rand_32() % 0xff), (uint8_t)(get_rand_32() % 0xff)},
        .unknown01 = 0x01,
        .storedColors = 0x02,
    };

    switchReport = {
        .reportID = 0x30,
        .timestamp = 0,

        .inputs{
            .connectionInfo = 0,
            .batteryLevel = 0x08,

            // byte 00
            .y = 0,
            .x = 0,
            .b = 0,
            .a = 0,
            .buttonRightSR = 0,
            .buttonRightSL = 0,
            .rightShoulder = 0,
            .rightTrigger = 0,

            // byte 01
            .back = 0,
            .start = 0,
            .leftThumbClick = 0,
            .rightThumbClick = 0,
            .guide = 0,
            .capture = 0,
            .dummy2 = 0,
            .chargingGrip = 0,

            // byte 02
            .dpadDown = 0,
            .dpadUp = 0,
            .dpadRight = 0,
            .dpadLeft = 0,
            .buttonLeftSL = 0,
            .buttonLeftSR = 0,
            .leftShoulder = 0,
            .leftTrigger = 0,
            .leftStickX = SWITCH_PRO_JOYSTICK_CENTER,
            .leftStickY = SWITCH_PRO_JOYSTICK_CENTER,
            .rightStickX = SWITCH_PRO_JOYSTICK_CENTER,
            .rightStickY = SWITCH_PRO_JOYSTICK_CENTER,
        },
        .rumbleReport = 0,
        .imuData = {0x00},
        .padding = {0x00}};

    last_report_timer = to_ms_since_boot(get_absolute_time());

    factoryConfig->leftStickCalibration.getRealMin(leftMinX, leftMinY);
    factoryConfig->leftStickCalibration.getCenter(leftCenX, leftCenY);
    factoryConfig->leftStickCalibration.getRealMax(leftMaxX, leftMaxY);
    factoryConfig->rightStickCalibration.getRealMin(rightMinX, rightMinY);
    factoryConfig->rightStickCalibration.getCenter(rightCenX, rightCenY);
    factoryConfig->rightStickCalibration.getRealMax(rightMaxX, rightMaxY);
}
void SwitchGamepadDevice::initialize()
{
}
bool SwitchGamepadDevice::sendReport(uint8_t reportID, void const *reportData, uint16_t reportLength)
{
    bool response = send_report(reportLength, reportID, reportData);
    if (last_report_counter < 255)
    {
        last_report_counter++;
    }
    else
    {
        last_report_counter = 0;
    }
    return response;
}
void SwitchGamepadDevice::process(bool full_poll)
{
    uint32_t now = to_ms_since_boot(get_absolute_time());
    reportSent = false;
    if (isReportQueued)
    {
        if ((now - last_report_timer) > SWITCH_PRO_KEEPALIVE_TIMER)
        {
            if (ready()) {
                sendReport(queuedReportID, report, 64);
            }
            isReportQueued = false;
            last_report_timer = now;
        }
        reportSent = true;
    }

    if (isReady && !reportSent)
    {
        for (const auto &mapping : mappings)
        {
            mapping->update(full_poll);
            mapping->update_switch((uint8_t*)&switchReport.inputs);
        }
        if ((now - last_report_timer) > SWITCH_PRO_KEEPALIVE_TIMER)
        {
            switchReport.timestamp = last_report_counter;
            void *inputReport = &switchReport;
            uint16_t report_size = sizeof(switchReport);
            if (memcmp(last_report, inputReport, report_size) != 0)
            {
                // HID ready + report sent, copy previous report
                if (ready() && sendReport(0, inputReport, report_size) == true)
                {
                    memcpy(last_report, inputReport, report_size);
                    reportSent = true;
                }

                last_report_timer = now;
            }
        }
    }
    else
    {
        if (!isInitialized)
        {
            // send identification
            sendIdentify();
            if (ready() && send_report(64, 0, report))
            {
                isInitialized = true;
                reportSent = true;
                printf("sent init!\r\n");
            }

            last_report_timer = now;
        }
    }
}

size_t SwitchGamepadDevice::compatible_section_descriptor(uint8_t *dest, size_t remaining)
{
    return 0;
}

size_t SwitchGamepadDevice::config_descriptor(uint8_t *dest, size_t remaining)
{
    if (!m_eps_assigned)
    {
        m_eps_assigned = true;
        m_epin = next_epin();
        m_epout = next_epout();
        usb_instances_by_epnum[m_epin] = usb_instances[interface_id];
        usb_instances_by_epnum[m_epout] = usb_instances[interface_id];
    }
    uint8_t desc[] = {TUD_HID_INOUT_DESCRIPTOR(interface_id, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report_switch_pro), m_epout, m_epin, CFG_TUD_HID_EP_BUFSIZE, 1)};
    assert(sizeof(desc) <= remaining);
    memcpy(dest, desc, sizeof(desc));
    return sizeof(desc);
}

void SwitchGamepadDevice::device_descriptor(tusb_desc_device_t *desc)
{
    desc->idVendor = NINTENDO_VID;
    desc->idProduct = SWITCH_PRO_PID;
}
const uint8_t *SwitchGamepadDevice::report_descriptor()
{
    return desc_hid_report_switch_pro;
}

uint16_t SwitchGamepadDevice::report_desc_len()
{
    return sizeof(desc_hid_report_switch_pro);
}
uint16_t SwitchGamepadDevice::get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
    printf("get report: %02x %02x %04x\r\n", report_id, report_type, reqlen);
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;

    return 0;
}

void SwitchGamepadDevice::set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
    if (report_type != HID_REPORT_TYPE_OUTPUT)
        return;

    memset(report, 0x00, bufsize);

    uint8_t switchReportID = buffer[0];
    uint8_t switchReportSubID = buffer[1];
    // printf("SwitchProDriver::set_report Rpt: %02x, Type: %d, Len: %d :: SID: %02x, SSID: %02x\n", report_id, report_type, bufsize, switchReportID, switchReportSubID);
    if (switchReportID == SwitchReportID::REPORT_OUTPUT_00)
    {
    }
    else if (switchReportID == SwitchReportID::REPORT_FEATURE)
    {
        queuedReportID = report_id;
        handleFeatureReport(switchReportID, switchReportSubID, buffer, bufsize);
    }
    else if (switchReportID == SwitchReportID::REPORT_CONFIGURATION)
    {
        queuedReportID = report_id;
        handleConfigReport(switchReportID, switchReportSubID, buffer, bufsize);
    }
    else
    {
        // printf("SwitchProDriver::set_report Rpt: %02x, Type: %d, Len: %d :: SID: %02x, SSID: %02x\n", report_id, report_type, bufsize, switchReportID, switchReportSubID);
    }
}

void SwitchGamepadDevice::sendIdentify()
{
    memset(report, 0x00, 64);
    report[0] = SwitchReportID::REPORT_USB_INPUT_81;
    report[1] = SwitchOutputSubtypes::IDENTIFY;
    report[2] = 0x00;
    report[3] = deviceInfo.controllerType;
    // MAC address
    for (uint8_t i = 0; i < 6; i++)
    {
        report[4 + i] = deviceInfo.macAddress[5 - i];
    }
}

void SwitchGamepadDevice::handleConfigReport(uint8_t switchReportID, uint8_t switchReportSubID, const uint8_t *reportData, uint16_t reportLength)
{
    bool canSend = false;
    printf("SwitchProDriver::handleConfigReport: %02x\r\n", switchReportSubID);
    switch (switchReportSubID)
    {
    case SwitchOutputSubtypes::IDENTIFY:
        // printf("SwitchProDriver::set_report: IDENTIFY\n");
        sendIdentify();
        canSend = true;
        break;
    case SwitchOutputSubtypes::HANDSHAKE:
        // printf("SwitchProDriver::set_report: HANDSHAKE\n");
        report[0] = SwitchReportID::REPORT_USB_INPUT_81;
        report[1] = SwitchOutputSubtypes::HANDSHAKE;
        canSend = true;
        break;
    case SwitchOutputSubtypes::BAUD_RATE:
        // printf("SwitchProDriver::set_report: BAUD_RATE\n");
        report[0] = SwitchReportID::REPORT_USB_INPUT_81;
        report[1] = SwitchOutputSubtypes::BAUD_RATE;
        canSend = true;
        break;
    case SwitchOutputSubtypes::DISABLE_USB_TIMEOUT:
        // printf("SwitchProDriver::set_report: DISABLE_USB_TIMEOUT\n");
        report[0] = SwitchReportID::REPORT_OUTPUT_30;
        report[1] = switchReportSubID;
        // if (handshakeCounter < 4) {
        //     handshakeCounter++;
        // } else {
        isReady = true;
        //}
        canSend = true;
        break;
    case SwitchOutputSubtypes::ENABLE_USB_TIMEOUT:
        // printf("SwitchProDriver::set_report: ENABLE_USB_TIMEOUT\n");
        report[0] = SwitchReportID::REPORT_OUTPUT_30;
        report[1] = switchReportSubID;
        canSend = true;
        break;
    default:
        // printf("SwitchProDriver::set_report: Unknown Sub ID %02x\n", switchReportSubID);
        report[0] = SwitchReportID::REPORT_OUTPUT_30;
        report[1] = switchReportSubID;
        canSend = true;
        break;
    }

    if (canSend)
        isReportQueued = true;
}

void SwitchGamepadDevice::handleFeatureReport(uint8_t switchReportID, uint8_t switchReportSubID, const uint8_t *reportData, uint16_t reportLength)
{
    printf("SwitchProDriver::handleFeatureReport: %02x\r\n", switchReportSubID);
    uint8_t commandID = reportData[10];
    uint32_t spiReadAddress = 0;
    uint8_t spiReadSize = 0;
    bool canSend = false;

    // uint8_t inputReportSize = sizeof(SwitchInputReport);
    // printf("inputReportSize: %d\n", inputReportSize);

    report[0] = SwitchReportID::REPORT_OUTPUT_21;
    report[1] = last_report_counter;
    memcpy(report + 2, &switchReport.inputs, sizeof(SwitchInputReport));

    switch (commandID)
    {
    case SwitchCommands::GET_CONTROLLER_STATE:
        // printf("SwitchProDriver::set_report: Rpt 0x01 GET_CONTROLLER_STATE\n");
        report[13] = 0x80;
        report[14] = commandID;
        report[15] = 0x03;
        canSend = true;
        break;
    case SwitchCommands::BLUETOOTH_PAIR_REQUEST:
        // printf("SwitchProDriver::set_report: Rpt 0x01 BLUETOOTH_PAIR_REQUEST\n");
        report[13] = 0x81;
        report[14] = commandID;
        report[15] = 0x03;
        canSend = true;
        break;
    case SwitchCommands::REQUEST_DEVICE_INFO:
        // printf("SwitchProDriver::set_report: Rpt 0x01 REQUEST_DEVICE_INFO\n");
        report[13] = 0x82;
        report[14] = 0x02;
        memcpy(&report[15], &deviceInfo, sizeof(deviceInfo));
        canSend = true;
        break;
    case SwitchCommands::SET_MODE:
        // printf("SwitchProDriver::set_report: Rpt 0x01 SET_MODE\n");
        inputMode = reportData[11];
        report[13] = 0x80;
        report[14] = 0x03;
        report[15] = inputMode;
        canSend = true;
        // printf("Input Mode set to ");
        switch (inputMode)
        {
        case 0x00:
            // printf("NFC/IR Polling Data");
            break;
        case 0x01:
            // printf("NFC/IR Polling Config");
            break;
        case 0x02:
            // printf("NFC/IR Polling Data+Config");
            break;
        case 0x03:
            // printf("IR Scan");
            break;
        case 0x23:
            // printf("MCU Update");
            break;
        case 0x30:
            // printf("Full Input");
            break;
        case 0x31:
            // printf("NFC/IR");
            break;
        case 0x3F:
            // printf("Simple HID");
            break;
        case 0x33:
        case 0x35:
        default:
            // printf("Unknown");
            break;
        }
        // printf("\n");
        break;
    case SwitchCommands::TRIGGER_BUTTONS:
        // printf("SwitchProDriver::set_report: Rpt 0x01 TRIGGER_BUTTONS\n");
        report[13] = 0x83;
        report[14] = 0x04;
        canSend = true;
        break;
    case SwitchCommands::SET_SHIPMENT:
        // printf("SwitchProDriver::set_report: Rpt 0x01 SET_SHIPMENT\n");
        report[13] = 0x80;
        report[14] = commandID;
        canSend = true;
        // for (uint8_t i = 2; i < bufsize; i++) {
        //     //printf("%02x ", reportData[i]);
        // }
        // printf("\n");
        break;
    case SwitchCommands::SPI_READ:
        // printf("SwitchProDriver::set_report: Rpt 0x01 SPI_READ\n");
        spiReadAddress = (reportData[14] << 24) | (reportData[13] << 16) | (reportData[12] << 8) | (reportData[11]);
        spiReadSize = reportData[15];
        // printf("Read From: 0x%08x Size %d\n", spiReadAddress, spiReadSize);
        report[13] = 0x90;
        report[14] = reportData[10];
        report[15] = reportData[11];
        report[16] = reportData[12];
        report[17] = reportData[13];
        report[18] = reportData[14];
        report[19] = reportData[15];
        readSPIFlash(&report[20], spiReadAddress, spiReadSize);
        canSend = true;
        // printf("----------------------------------------------\n");
        break;
    case SwitchCommands::SET_NFC_IR_CONFIG:
        // printf("SwitchProDriver::set_report: Rpt 0x01 SET_NFC_IR_CONFIG\n");
        report[13] = 0x80;
        report[14] = commandID;
        canSend = true;
        break;
    case SwitchCommands::SET_NFC_IR_STATE:
        // printf("SwitchProDriver::set_report: Rpt 0x01 SET_NFC_IR_STATE\n");
        report[13] = 0x80;
        report[14] = commandID;
        canSend = true;
        break;
    case SwitchCommands::SET_PLAYER_LIGHTS:
        // printf("SwitchProDriver::set_report: Rpt 0x01 SET_PLAYER_LIGHTS\n");
        playerID = reportData[11];
        report[13] = 0x80;
        report[14] = commandID;
        canSend = true;
        // printf("Player set to %d\n", playerID);
        // printf("----------------------------------------------\n");
        break;
    case SwitchCommands::GET_PLAYER_LIGHTS:
        // printf("SwitchProDriver::set_report: Rpt 0x01 GET_PLAYER_LIGHTS\n");
        playerID = reportData[11];
        report[13] = 0xB0;
        report[14] = commandID;
        report[15] = playerID;
        canSend = true;
        // printf("Player is %d\n", playerID);
        // printf("----------------------------------------------\n");
        break;
    case SwitchCommands::COMMAND_UNKNOWN_33:
        // printf("SwitchProDriver::set_report: Rpt 0x01 COMMAND_UNKNOWN_33\n");
        //  Command typically thrown by Chromium to detect if a Switch controller exists. Can ignore.
        report[13] = 0x80;
        report[14] = commandID;
        report[15] = 0x03;
        canSend = true;
        break;
    case SwitchCommands::SET_HOME_LIGHT:
        // printf("SwitchProDriver::set_report: Rpt 0x01 SET_HOME_LIGHT\n");
        //  NYI
        report[13] = 0x80;
        report[14] = commandID;
        report[15] = 0x00;
        canSend = true;
        break;
    case SwitchCommands::TOGGLE_IMU:
        // printf("SwitchProDriver::set_report: Rpt 0x01 TOGGLE_IMU\n");
        isIMUEnabled = reportData[11];
        report[13] = 0x80;
        report[14] = commandID;
        report[15] = 0x00;
        canSend = true;
        // printf("IMU set to %d\n", isIMUEnabled);
        // printf("----------------------------------------------\n");
        break;
    case SwitchCommands::IMU_SENSITIVITY:
        // printf("SwitchProDriver::set_report: Rpt 0x01 IMU_SENSITIVITY\n");
        report[13] = 0x80;
        report[14] = commandID;
        canSend = true;
        break;
    case SwitchCommands::ENABLE_VIBRATION:
        // printf("SwitchProDriver::set_report: Rpt 0x01 ENABLE_VIBRATION\n");
        isVibrationEnabled = reportData[11];
        report[13] = 0x80;
        report[14] = commandID;
        report[15] = 0x00;
        canSend = true;
        // printf("Vibration set to %d\n", isVibrationEnabled);
        // printf("----------------------------------------------\n");
        break;
    case SwitchCommands::READ_IMU:
        // printf("SwitchProDriver::set_report: Rpt 0x01 READ_IMU\n");
        report[13] = 0xC0;
        report[14] = commandID;
        report[15] = reportData[11];
        report[16] = reportData[12];
        canSend = true;
        // printf("IMU Addr: %02x, Size: %02x\n", reportData[11], reportData[12]);
        // printf("----------------------------------------------\n");
        break;
    case SwitchCommands::GET_VOLTAGE:
        // printf("SwitchProDriver::set_report: Rpt 0x01 GET_VOLTAGE\n");
        report[13] = 0xD0;
        report[14] = 0x50;
        report[15] = 0x83;
        report[16] = 0x06;
        canSend = true;
        break;
    default:
        // printf("SwitchProDriver::set_report: Rpt 0x01 Unknown 0x%02x\n", commandID);
        report[13] = 0x80;
        report[14] = commandID;
        report[15] = 0x03;
        canSend = true;
        break;
    }

    if (canSend)
        isReportQueued = true;
}
void SwitchGamepadDevice::readSPIFlash(uint8_t *dest, uint32_t address, uint8_t size)
{
    uint32_t addressBank = address & 0xFFFFFF00;
    uint32_t addressOffset = address & 0x000000FF;
    // printf("Address: %08x, Bank: %04x, Offset: %04x, Size: %d\n", address, addressBank, addressOffset, size);
    std::map<uint32_t, const uint8_t *>::iterator it = spiFlashData.find(addressBank);

    if (it != spiFlashData.end())
    {
        // address found
        const uint8_t *data = it->second;
        memcpy(dest, data + addressOffset, size);
        // for (uint8_t i = 0; i < size; i++) printf("%02x ", dest[i]);
        // printf("\n---\n");
    }
    else
    {
        // could not find defined address
        // printf("Not Found\n");
        memset(dest, 0xFF, size);
    }
}