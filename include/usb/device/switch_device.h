
#pragma once
#include "usb/usb_descriptors.h"
#include "hid_device.h"

#include "tusb.h"
#define SWITCH_PRO_KEEPALIVE_TIMER 5
#define SWITCH_PRO_JOYSTICK_CENTER 0x07FF
typedef enum {
    REPORT_OUTPUT_00 = 0x00,
    REPORT_FEATURE = 0x01,
    REPORT_OUTPUT_10 = 0x10,
    REPORT_OUTPUT_21 = 0x21,
    REPORT_OUTPUT_30 = 0x30,
    REPORT_CONFIGURATION = 0x80,
    REPORT_USB_INPUT_81 = 0x81,
} SwitchReportID;

typedef enum {
    IDENTIFY = 0x01,
    HANDSHAKE,
    BAUD_RATE,
    DISABLE_USB_TIMEOUT,
    ENABLE_USB_TIMEOUT
} SwitchOutputSubtypes;

typedef enum {
    GET_CONTROLLER_STATE = 0x00,
    BLUETOOTH_PAIR_REQUEST = 0x01,
    REQUEST_DEVICE_INFO = 0x02,
    SET_MODE = 0x03,
    TRIGGER_BUTTONS = 0x04,
    SET_SHIPMENT = 0x08,
    SPI_READ = 0x10,
    SET_NFC_IR_CONFIG = 0x21,
    SET_NFC_IR_STATE = 0x22,
    SET_PLAYER_LIGHTS = 0x30,
    GET_PLAYER_LIGHTS = 0x31,
    COMMAND_UNKNOWN_33 = 0x33,
    SET_HOME_LIGHT = 0x38,
    TOGGLE_IMU = 0x40,
    IMU_SENSITIVITY = 0x41,
    READ_IMU = 0x43,
    ENABLE_VIBRATION = 0x48,
    GET_VOLTAGE = 0x50,
} SwitchCommands;

static const uint8_t SWITCH_INIT_REPORT[10] = {SwitchReportID::REPORT_CONFIGURATION, SwitchOutputSubtypes::IDENTIFY};


// left and right calibration are stored differently for some reason, so two structs
typedef struct {
    uint8_t data[9];

    void getMin(uint16_t& x, uint16_t& y) { packCalib(6, x, y); }
    void getCenter(uint16_t& x, uint16_t& y) { packCalib(3, x, y); }
    void getMax(uint16_t& x, uint16_t& y) { packCalib(0, x, y); }

    void getRealMin(uint16_t& x, uint16_t& y) {
        uint16_t minX, minY;
        uint16_t cenX, cenY;

        getMin(minX, minY);
        getCenter(cenX, cenY);

        x = cenX - minX;
        y = cenY - minY;
    }

    void getRealMax(uint16_t& x, uint16_t& y) {
        uint16_t maxX, maxY;
        uint16_t cenX, cenY;

        getMax(maxX, maxY);
        getCenter(cenX, cenY);

        x = cenX + maxX;
        y = cenY + maxY;
    }

    void packCalib(uint8_t offset, uint16_t& x, uint16_t& y) {
        x = static_cast<uint16_t>(data[offset]) | ((data[offset + 1] & 0x0F) << 8);
        y = static_cast<uint16_t>(data[offset + 2] << 4) | (data[offset + 1] >> 4);
    }
} SwitchLeftCalibration;

typedef struct {
    uint8_t data[9];

    void getMin(uint16_t& x, uint16_t& y) { packCalib(3, x, y); }
    void getCenter(uint16_t& x, uint16_t& y) { packCalib(0, x, y); }
    void getMax(uint16_t& x, uint16_t& y) { packCalib(6, x, y); }

    void getRealMin(uint16_t& x, uint16_t& y) {
        uint16_t minX, minY;
        uint16_t cenX, cenY;

        getMin(minX, minY);
        getCenter(cenX, cenY);

        x = cenX - minX;
        y = cenY - minY;
    }

    void getRealMax(uint16_t& x, uint16_t& y) {
        uint16_t maxX, maxY;
        uint16_t cenX, cenY;

        getMax(maxX, maxY);
        getCenter(cenX, cenY);

        x = cenX + maxX;
        y = cenY + maxY;
    }

    void packCalib(uint8_t offset, uint16_t& x, uint16_t& y) {
        x = static_cast<uint16_t>(data[offset]) | ((data[offset + 1] & 0x0F) << 8);
        y = static_cast<uint16_t>(data[offset + 2] << 4) | (data[offset + 1] >> 4);
    }
} SwitchRightCalibration;

typedef struct
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} SwitchColorDefinition;

typedef struct __attribute((packed, aligned(1)))
{
    uint8_t serialNumber[16];

    uint8_t unknown00[2];

    uint8_t deviceType;

    uint8_t unknown01; // usually 0xA0

    uint8_t unknown02[7];

    uint8_t colorInfo; // 0 = default colors

    uint8_t unknown03[4];

    uint8_t motionCalibration[24];

    uint8_t unknown04[5];

    SwitchLeftCalibration leftStickCalibration;

    SwitchRightCalibration rightStickCalibration;

    uint8_t unknown08;

    SwitchColorDefinition bodyColor;

    SwitchColorDefinition buttonColor;

    SwitchColorDefinition leftGripColor;

    SwitchColorDefinition rightGripColor;

    uint8_t unknown06[37];

    uint8_t motionHorizontalOffsets[6];

    uint8_t stickParams1[17];

    uint8_t stickParams2[17];

    uint8_t unknown07[0xE57];
} SwitchFactoryConfig;

typedef struct __attribute((packed, aligned(1)))
{
    uint8_t unknown00[16];

    uint8_t leftCalibrationMagic[2];
    SwitchLeftCalibration leftCalibration;

    uint8_t rightCalibrationMagic[2];
    SwitchRightCalibration rightCalibration;

    uint8_t motionCalibrationMagic[2];
    uint8_t motionCalibration[24];
} SwitchUserCalibration;


typedef enum {
    SWITCH_TYPE_LEFT_JOYCON = 0x01,
    SWITCH_TYPE_RIGHT_JOYCON,
    SWITCH_TYPE_PRO_CONTROLLER,
    SWITCH_TYPE_FAMICOM_LEFT_JOYCON = 0x07,
    SWITCH_TYPE_FAMICOM_RIGHT_JOYCON = 0x08,
    SWITCH_TYPE_NES_LEFT_JOYCON = 0x09,
    SWITCH_TYPE_NES_RIGHT_JOYCON = 0x0A,
    SWITCH_TYPE_SNES = 0x0B,
    SWITCH_TYPE_N64 = 0x0C,
} SwitchControllerType;

typedef struct {
    uint8_t majorVersion;
    uint8_t minorVersion;
    uint8_t controllerType;
    uint8_t unknown00;
    uint8_t macAddress[6];
    uint8_t unknown01;
    uint8_t storedColors;
} SwitchDeviceInfo;

typedef struct
{
	uint16_t buttons;
	uint8_t hat;
	uint8_t lx;
	uint8_t ly;
	uint8_t rx;
	uint8_t ry;
} SwitchProOutReport;

class SwitchGamepadDevice : public HIDDevice
{
public:
    SwitchGamepadDevice();
    void initialize();
    void process(bool full_poll);
    size_t compatible_section_descriptor(uint8_t *desc, size_t remaining);
    size_t config_descriptor(uint8_t *desc, size_t remaining);
    void device_descriptor(tusb_desc_device_t *desc);
    const uint8_t *report_descriptor();
    uint16_t report_desc_len();
    uint16_t get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen);
    void set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize);
private:
    void readSPIFlash(uint8_t* dest, uint32_t address, uint8_t size);
    void handleFeatureReport(uint8_t switchReportID, uint8_t switchReportSubID, const uint8_t *reportData, uint16_t reportLength);
    void handleConfigReport(uint8_t switchReportID, uint8_t switchReportSubID, const uint8_t *reportData, uint16_t reportLength);
    void sendIdentify();
    void endSubCommand(uint8_t subCommand);
    bool sendReport(uint8_t reportID, void const* reportData, uint16_t reportLength);
    uint8_t report[CFG_TUD_HID_EP_BUFSIZE] = { };
    uint8_t last_report[CFG_TUD_HID_EP_BUFSIZE] = { };
    SwitchProGamepad_Data_t switchReport;
    uint8_t last_report_counter;
    uint32_t last_report_timer;
    bool isReady = false;
    bool isInitialized = false;
    bool isReportQueued = false;
    bool reportSent = false;
    uint8_t queuedReportID = 0;

    uint8_t handshakeCounter = 0;

    std::map<uint32_t, const uint8_t*> spiFlashData = {
        {0x6000, factoryConfigData},
        {0x8000, userCalibrationData}
    };

    SwitchDeviceInfo deviceInfo;
    uint8_t playerID = 0;
    uint8_t inputMode = 0x30;
    bool isIMUEnabled = false;
    bool isVibrationEnabled = false;
    uint16_t scale12To16(uint16_t pos) { return pos << 4; }
    uint16_t scale16To12(uint16_t pos) { return pos >> 4; }
    uint16_t map(uint16_t x, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max) { return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min; }

    uint16_t leftMinX, leftMinY;
    uint16_t leftCenX, leftCenY;
    uint16_t leftMaxX, leftMaxY;
    uint16_t rightMinX, rightMinY;
    uint16_t rightCenX, rightCenY;
    uint16_t rightMaxX, rightMaxY;

    // config data
    SwitchFactoryConfig* factoryConfig = (SwitchFactoryConfig*)factoryConfigData;
    const uint8_t factoryConfigData[0xEFF] = {
        // serial number
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

        0xFF, 0xFF, 

        // device type
        SwitchControllerType::SWITCH_TYPE_PRO_CONTROLLER, 

        // unknown
        0xA0, 

        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 

        // color options
        0x02,

        0xFF, 0xFF, 0xFF, 0xFF, 

        // config & calibration 1
        0xE3, 0xFF, 0x39, 0xFF, 0xED, 0x01, 0x00, 0x40,
        0x00, 0x40, 0x00, 0x40, 0x09, 0x00, 0xEA, 0xFF,
        0xA1, 0xFF, 0x3B, 0x34, 0x3B, 0x34, 0x3B, 0x34,

        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 

        // config & calibration 2
        // left stick
        0xa4, 0x46, 0x6a, 0x00, 0x08, 0x80, 0xa4, 0x46, 
        0x6a,

        // right stick
        0x00, 0x08, 0x80, 0xa4, 0x46, 0x6a, 0xa4, 0x46,
        0x6a,

        0xFF,

        // body color
        0x1B, 0x1B, 0x1D,

        // button color
        0xFF, 0xFF, 0xFF,

        // left grip color
        0xEC, 0x00, 0x8C,

        // right grip color
        0xEC, 0x00, 0x8C,

        0x01, 

        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF,
        
        0x50, 0xFD, 0x00, 0x00, 0xC6, 0x0F, 
        0x0F, 0x30, 0x61, 0xAE, 0x90, 0xD9, 0xD4, 0x14, 
        0x54, 0x41, 0x15, 0x54, 0xC7, 0x79, 0x9C, 0x33, 
        0x36, 0x63, 
        
        0x0F, 0x30, 0x61, 0xAE, 0x90, 0xD9, 0xD4, 0x14, 
        0x54, 0x41, 0x15, 0x54,
        
        0xC7,

        0x79, 
        
        0x9C, 

        0x33, 
        
        0x36, 
        
        0x63, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF
    };

    SwitchUserCalibration* userCalibration = (SwitchUserCalibration*)userCalibrationData;
    const uint8_t userCalibrationData[0x3F] = {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        
        // Left Stick
        0xB2, 0xA1, 0xa4, 0x46, 0x6a, 0x00, 0x08, 0x80, 
        0xa4, 0x46, 0x6a,

        // Right Stick
        0xB2, 0xA1, 0x00, 0x08, 0x80, 0xa4, 0x46, 0x6a,
        0xa4, 0x46, 0x6a,

        // Motion
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
    };
};