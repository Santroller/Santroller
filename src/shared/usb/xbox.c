#include "xbox.h"
#include "wcid.h"
#include "stdint.h"
#include "progmem.h"
#include "endpoints.h"
const uint8_t PROGMEM XBOX_ID[] = {0x00, 0x82, 0xf8, 0x23};
const uint8_t PROGMEM capabilities1[] = {0x00, 0x08, 0x00, 0x00,
                                         0x00, 0x00, 0x00, 0x00};
const uint8_t PROGMEM capabilities2[] = {0x00, 0x14, 0x3f, 0xf7, 0xff, 0xff, 0x00,
                                         0x00, 0x00, 0x00, 0xc0, 0xff, 0xc0, 0xff,
                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

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
             'D', '7', '0', '0', 'A', '3', '1', '3', 'D', '7', '8', '}', '\0'}
    }
};
const OS_COMPATIBLE_ID_DESCRIPTOR PROGMEM DevCompatIDs = {
    TotalLength : sizeof(OS_COMPATIBLE_ID_DESCRIPTOR),
    Version : 0x0100,
    Index : DESC_EXTENDED_COMPATIBLE_ID_DESCRIPTOR,
    TotalSections : 2,
    Reserved : {0},
    CompatID : {
        {
            FirstInterfaceNumber : INTERFACE_ID_Config,
            Reserved : 0x04,
            CompatibleID : "WINUSB",
            SubCompatibleID : {0},
            Reserved2 : {0}
        },
        {
            FirstInterfaceNumber : INTERFACE_ID_Device,
            Reserved : 0x04,
            CompatibleID : "XUSB10",
            SubCompatibleID : {0},
            Reserved2 : {0}
        }}
};