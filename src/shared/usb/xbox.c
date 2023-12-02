#include "xbox.h"

#include "endpoints.h"
#include "progmem.h"
#include "reports/xinput_reports.h"
#include "stdint.h"
#include "wcid.h"
#include "config.h"
const XInputVibrationCapabilities_t PROGMEM capabilities1 = {
    rid : 0x00,
    rsize : sizeof(XInputVibrationCapabilities_t),
    padding : 0x00,
    left_motor : 0xFF,
    right_motor : 0xFF,
    padding_2 : {0x00, 0x00, 0x00}
};
const XInputInputCapabilities_t PROGMEM capabilities2 = {
    rid : 0x00,
    rsize : sizeof(XInputInputCapabilities_t),
    buttons : 0xf73f,
    leftTrigger : 0xff,
    rightTrigger : 0xff,
    leftThumbX : 0x1209,
    leftThumbY : 0x2882,
    rightThumbX : USB_VERSION_BCD(DEVICE_TYPE, 0, 0),
    rightThumbY : 0xffc0,
    reserved : {0x00, 0x00, 0x00, 0x00},
    flags : 0x0000
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