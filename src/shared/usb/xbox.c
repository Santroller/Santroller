#include "xbox.h"

#include "config.h"
#include "endpoints.h"
#include "progmem.h"
#include "reports/xinput_reports.h"
#include "stdint.h"
#include "wcid.h"
#include "ids.h"
const XInputVibrationCapabilities_t PROGMEM XInputVibrationCapabilities = {
    rid : 0x00,
    rsize : sizeof(XInputVibrationCapabilities_t),
    padding : 0x00,
    left_motor : 0xFF,
    right_motor : 0xFF,
    padding_2 : {0x00, 0x00, 0x00}
};
const XInputInputCapabilities_t PROGMEM XInputInputCapabilities = {
    rid : 0x00,
    rsize : sizeof(XInputInputCapabilities_t),
    buttons : 0xf73f,
    leftTrigger : 0xff,
    rightTrigger : 0xff,
    leftThumbX : ARDWIINO_VID,
    leftThumbY : ARDWIINO_PID,
    rightThumbX : 0,
    rightThumbY : 0xffc0,
    reserved : {0x00, 0x00, 0x00, 0x00},
    flags : XINPUT_FLAGS_FORCE_FEEDBACK
};

const OS_EXTENDED_COMPATIBLE_ID_DESCRIPTOR PROGMEM ExtendedIDs = {
    TotalLength : sizeof(OS_EXTENDED_COMPATIBLE_ID_DESCRIPTOR),
    Version : 0x0100,
    Index : DESC_EXTENDED_PROPERTIES_DESCRIPTOR,
    TotalSections : 1,
    SectionSize : 132,
    ExtendedID : {
        PropertyDataType : 1,
        PropertyNameLength : 40,
        PropertyName : {'D', 'e', 'v', 'i', 'c', 'e', 'I', 'n', 't', 'e',
                        'r', 'f', 'a', 'c', 'e', 'G', 'U', 'I', 'D', '\0'},
        PropertyDataLength : 78,
        PropertyData :
            {'{', 'D', 'F', '5', '9', '0', '3', '7', 'D', '-', '7', 'C', '9',
             '2', '-', '4', '1', '5', '5', '-', 'A', 'C', '1', '2', '-', '7',
             'D', '7', '0', '0', 'A', '3', '1', '3', 'D', '7', '9', '}', '\0'}
    }
};
const OS_COMPATIBLE_ID_DESCRIPTOR_SINGLE PROGMEM DevCompatIDsPS3 = {
    TotalLength : sizeof(OS_COMPATIBLE_ID_DESCRIPTOR_SINGLE),
    Version : 0x0100,
    Index : DESC_EXTENDED_COMPATIBLE_ID_DESCRIPTOR,
    TotalSections : 1,
    Reserved : {0},
    CompatID : {
        {
            FirstInterfaceNumber : INTERFACE_ID_Device,
            Reserved : 0x01,
            CompatibleID : "WINUSB",
            SubCompatibleID : {0},
            Reserved2 : {0}
        }}
};

const OS_COMPATIBLE_ID_DESCRIPTOR_THREE PROGMEM DevCompatIDsUniversal = {
    TotalLength : sizeof(OS_COMPATIBLE_ID_DESCRIPTOR_THREE),
    Version : 0x0100,
    Index : DESC_EXTENDED_COMPATIBLE_ID_DESCRIPTOR,
    TotalSections : 3,
    Reserved : {0},
    CompatID : {
        {
            FirstInterfaceNumber : INTERFACE_ID_Config,
            Reserved : 0x01,
            CompatibleID : "WINUSB",
            SubCompatibleID : {0},
            Reserved2 : {0}
        },
        {
            FirstInterfaceNumber : INTERFACE_ID_Padding,
            Reserved : 0x01,
            CompatibleID : "XUSB10",
            SubCompatibleID : {0},
            Reserved2 : {0}
        },
        {
            FirstInterfaceNumber : INTERFACE_ID_XBOX_Security,
            Reserved : 0x01,
            CompatibleID : "XUSB10",
            SubCompatibleID : {0},
            Reserved2 : {0}
        }}
};

const OS_COMPATIBLE_ID_DESCRIPTOR PROGMEM DevCompatIDs = {
    TotalLength : sizeof(OS_COMPATIBLE_ID_DESCRIPTOR_SINGLE),
    Version : 0x0100,
    Index : DESC_EXTENDED_COMPATIBLE_ID_DESCRIPTOR,
    TotalSections : 1,
    Reserved : {0},
    CompatID : {
        {
            FirstInterfaceNumber : INTERFACE_ID_Config,
            Reserved : 0x01,
            CompatibleID : "WINUSB",
            SubCompatibleID : {0},
            Reserved2 : {0}
        },
        {
            FirstInterfaceNumber : INTERFACE_ID_Device,
            Reserved : 0x01,
            CompatibleID : "XUSB10",
            SubCompatibleID : {0},
            Reserved2 : {0}
        },
        {
            FirstInterfaceNumber : INTERFACE_ID_Padding,
            Reserved : 0x01,
            CompatibleID : "XUSB10",
            SubCompatibleID : {0},
            Reserved2 : {0}
        },
        {
            FirstInterfaceNumber : INTERFACE_ID_XBOX_Security,
            Reserved : 0x01,
            CompatibleID : "XUSB10",
            SubCompatibleID : {0},
            Reserved2 : {0}
        }}
};

const OS_COMPATIBLE_ID_DESCRIPTOR_SINGLE PROGMEM DevCompatIDsOne = {
    TotalLength : sizeof(OS_COMPATIBLE_ID_DESCRIPTOR_SINGLE),
    Version : 0x0100,
    Index : DESC_EXTENDED_COMPATIBLE_ID_DESCRIPTOR,
    TotalSections : 1,
    Reserved : {0},
    CompatID : {
        {
            Reserved : 0x01,
            CompatibleID : "XGIP10",
            SubCompatibleID : {0},
            Reserved2 : {0}
        }}
};

const XID_DESCRIPTOR PROGMEM DukeXIDDescriptor = {
    bLength : sizeof(XID_DESCRIPTOR),
    bDescriptorType : 0x42,
    bcdXid : USB_VERSION_BCD(1, 0, 0),
    bType : 0x01,
    bSubType : 0x02,
    bMaxInputReportSize : sizeof(OGXboxGamepad_Data_t),
    bMaxOutputReportSize : sizeof(OGXboxOutput_Report_t),
    wAlternateProductIds : {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}
};

const OGXboxGamepadCapabilities_Data_t PROGMEM DukeXIDInputCapabilities = {
    rid : 0x00,
    rsize : sizeof(OGXboxGamepadCapabilities_Data_t),
    dpadUp : 1,
    dpadDown : 1,
    dpadLeft : 1,
    dpadRight : 1,
    start : 1,
    back : 1,
    leftThumbClick : 1,
    rightThumbClick : 1,
    padding : 0,
    a : 0xFF,
    b : 0xFF,
    x : 0xFF,
    y : 0xFF,
    leftShoulder : 0xFF,
    rightShoulder : 0xFF,
    leftTrigger : 0xFF,
    rightTrigger : 0xFF,
    leftStickX : UINT16_MAX,
    leftStickY : UINT16_MAX,
    rightStickX : UINT16_MAX,
    rightStickY : UINT16_MAX,
};

const OGXboxOutput_Report_t PROGMEM DukeXIDVibrationCapabilities = {
    rid : 0x00,
    rsize : sizeof(OGXboxOutput_Report_t),
    left : 0xFFFF,
    right : 0xFFFF
};