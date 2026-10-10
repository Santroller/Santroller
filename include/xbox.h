#include <stdint.h>
#include "wcid.h"
#include "reports/xinput_reports.h"
#include "reports/og_xbox_reports.h"
typedef struct {
    uint8_t bLength; // Length of this descriptor.
    uint8_t bDescriptorType; // CONFIGURATION descriptor type (USB_DESCRIPTOR_CONFIGURATION).
    uint8_t reserved[2];
    uint8_t subtype;
    uint8_t reserved2;
    uint8_t bEndpointAddressIn;
    uint8_t bMaxDataSizeIn;
    uint8_t reserved3[5];
    uint8_t bEndpointAddressOut;
    uint8_t bMaxDataSizeOut;
    uint8_t reserved4[2];
} __attribute__((packed)) XBOX_ID_DESCRIPTOR;

typedef struct {
    uint8_t bLength; // Length of this descriptor.
    uint8_t bDescriptorType; // CONFIGURATION descriptor type (USB_DESCRIPTOR_CONFIGURATION).
    uint8_t reserved[18];
} __attribute__((packed)) XBOX_ID_W_DESCRIPTOR;
typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bcdXid;
    uint8_t bType;
    uint8_t bSubType;
    uint8_t bMaxInputReportSize;
    uint8_t bMaxOutputReportSize;
    uint16_t wAlternateProductIds[4];
} __attribute__((packed)) XID_DESCRIPTOR;

typedef struct {
    uint8_t id;
    uint8_t type;
    uint8_t unknown;
    uint16_t state;
} __attribute__((packed)) XBOX_WIRELESS_HEADER;

typedef struct {
    uint8_t id;
    uint8_t type;
    uint8_t unknown1;
    // byte 0
    uint8_t vibrationLevel : 2;
    uint8_t headset : 1;
    uint8_t chatpad : 1;
    uint8_t always_0x1 : 4;
    
    // byte 1
    uint8_t unknown : 1;
    uint8_t batteryType : 2;
    uint8_t onlyMic : 1;
    uint8_t powerState : 2;
    uint8_t batteryLevel : 2;
} __attribute__((packed)) XBOX_WIRELESS_STATE;

typedef struct {
    XBOX_WIRELESS_HEADER header;
    uint8_t always_0xCC;
    uint32_t unk1;
    uint32_t deviceID;
    uint8_t type;
    uint8_t revision;
    uint8_t state[2];
    uint16_t protocol;
    uint8_t unk2[2];
    uint8_t vendorIDData[3];
    uint8_t subtype;
    uint8_t unk3[3];
} __attribute__((packed)) XBOX_WIRELESS_LINK_REPORT;

typedef struct {
    XBOX_WIRELESS_HEADER header;
    uint8_t always_0x12;
    uint16_t buttons;
    uint8_t leftTrigger;
    uint8_t rightTrigger;
    uint16_t leftStickX;
    uint16_t leftStickY;
    uint16_t rightStickX;
    uint16_t rightStickY;
    uint8_t leftMotor;
    uint8_t rightMotor;
    uint8_t unk[9];
} __attribute__((packed)) XBOX_WIRELESS_CAPABILITIES;

extern const XInputVibrationCapabilities_t XInputVibrationCapabilities;
extern const XInputInputCapabilities_t XInputInputCapabilities;
extern const OS_EXTENDED_COMPATIBLE_ID_DESCRIPTOR ExtendedIDs;
extern const OS_COMPATIBLE_ID_DESCRIPTOR DevCompatIDs;  
extern const OS_COMPATIBLE_ID_DESCRIPTOR_THREE DevCompatIDsUniversal;
extern const OS_COMPATIBLE_ID_DESCRIPTOR_SINGLE DevCompatIDsPS3;
extern const OS_COMPATIBLE_ID_DESCRIPTOR_SINGLE DevCompatIDsOne; 
extern const OS_COMPATIBLE_ID_DESCRIPTOR_SINGLE DevCompatIDsOneDetect;

extern const XID_DESCRIPTOR DukeXIDDescriptor;
extern const OGXboxGamepadCapabilities_Data_t DukeXIDInputCapabilities;
extern const OGXboxOutput_Report_t DukeXIDVibrationCapabilities;