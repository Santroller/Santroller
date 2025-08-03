/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef USB_DESCRIPTORS_H_
#define USB_DESCRIPTORS_H_
#include <stdint.h>
#define USB_PID 0x2882

#define USB_VID 0x1209
#define USB_BCD 0x2882

enum descriptors_t {
    DESC_EXTENDED_COMPATIBLE_ID_DESCRIPTOR = 0x0004,
    DESC_EXTENDED_PROPERTIES_DESCRIPTOR = 0x0005,
};

enum requests_t {
    REQ_GET_OS_FEATURE_DESCRIPTOR = 0x20
};
typedef struct {
    uint8_t bLength;         /**< Size of the descriptor, in bytes. */
    uint8_t bDescriptorType; /**< Type of the descriptor, either a value in \ref TUSB_DescriptorTypes_t or a value
				                          *   given by the specific class. */
    uint16_t Signature[7];
    uint8_t VendorCode;
    uint8_t Reserved;
} __attribute__((packed)) OS_DESCRIPTOR;

typedef struct {
    uint8_t FirstInterfaceNumber;
    uint8_t Reserved;
    uint8_t CompatibleID[8];
    uint8_t SubCompatibleID[8];
    uint8_t Reserved2[6];
} __attribute__((packed)) OS_COMPATIBLE_SECTION;

typedef struct {
    uint32_t TotalLength;
    uint16_t Version;
    uint16_t Index;
    uint8_t TotalSections;
    uint8_t Reserved[7];
    OS_COMPATIBLE_SECTION CompatID[4];
} __attribute__((packed)) OS_COMPATIBLE_ID_DESCRIPTOR;

typedef struct {
    uint32_t TotalLength;
    uint16_t Version;
    uint16_t Index;
    uint8_t TotalSections;
    uint8_t Reserved[7];
    OS_COMPATIBLE_SECTION CompatID[4];
} __attribute__((packed)) OS_COMPATIBLE_ID_DESCRIPTOR_SINGLE;

typedef struct {
    uint32_t PropertyDataType;
    uint16_t PropertyNameLength;
    uint16_t PropertyName[20];
    uint32_t PropertyDataLength;
    uint16_t PropertyData[39];
} __attribute__((packed)) OS_EXTENDED_SECTION;

typedef struct {
    uint32_t TotalLength;
    uint16_t Version;
    uint16_t Index;
    uint16_t TotalSections;
    uint32_t SectionSize;
    OS_EXTENDED_SECTION ExtendedID;
} __attribute__((packed)) OS_EXTENDED_COMPATIBLE_ID_DESCRIPTOR;
typedef struct {
    uint8_t type;
    uint8_t u1;
    uint8_t size_challenge;
    uint8_t size_response;
    uint8_t u4[4];  // crc32?
} __attribute__((packed)) AuthPageSizeReport;

typedef struct {
    uint8_t type;      // 0
    uint8_t seq;       // 1
    uint8_t page;      // 2
    uint8_t sbz;       // 3
    uint8_t data[56];  // 4-59
    uint32_t crc32;    // 60-63
} __attribute__((packed)) AuthReport;

typedef struct {
    uint8_t type;        // 0
    uint8_t seq;         // 1
    uint8_t status;      // 2  0x10 = not ready, 0x00 = ready
    uint8_t padding[9];  // 3-11
    uint32_t crc32;      // 12-15
} __attribute__((packed)) AuthStatusReport;

enum BackendAuthState {
    OK,
    UNKNOWN_ERR,
    COMM_ERR,
    BUSY,
    NO_TRANSACTION,
};
enum
{
  ITF_NUM_HID,
  ITF_NUM_XINPUT,
  ITF_NUM_XINPUT2,
//   ITF_NUM_XINPUT_AUDIO,
//   ITF_NUM_XINPUT_PLUGIN_MODULE,
  ITF_NUM_XINPUT_SECURITY,
  ITF_NUM_TOTAL
};

enum
{
    REPORT_ID_GAMEPAD = 1,
    REPORT_ID_KEYBOARD = 2,
    REPORT_ID_MOUSE = 3,
    REPORT_ID_CONSUMER = 4,
    REPORT_ID_CONFIG = 0x22,
    REPORT_ID_CONFIG_INFO = 0x23,
    REPORT_ID_PS3_02 = 0x02,
    REPORT_ID_PS3_EE = 0xEE,
    REPORT_ID_PS3_EF = 0xEF,
    REPORT_ID_PS3_F2 = 0xF2,
    REPORT_ID_PS3_F4 = 0xF4,
    REPORT_ID_PS3_F5 = 0xF5,
    REPORT_ID_PS3_F6 = 0xF6,
    REPORT_ID_PS3_F7 = 0xF7,
    REPORT_ID_PS3_F8 = 0xF8,
    REPORT_ID_PS4_FEATURE = 0x03,
    REPORT_ID_PS4_05 = 0x05,
    REPORT_ID_PS4_SET_CHALLENGE = 0xF0,
    REPORT_ID_PS4_GET_RESPONSE = 0xF1,
    REPORT_ID_PS4_GET_AUTH_STATUS = 0xF2,
    REPORT_ID_PS4_GET_AUTH_PAGE_SIZE = 0xF3,
    REPORT_ID_COUNT
};


// For turntables, both left and right are set to the same thing
// For these, left is set to 0, and right to these values
#define RUMBLE_STAGEKIT_FOG_ON 0x1
#define RUMBLE_STAGEKIT_FOG_OFF 0x2
#define RUMBLE_STAGEKIT_SLOW_STROBE 0x3
#define RUMBLE_STAGEKIT_MEDIUM_STROBE 0x4
#define RUMBLE_STAGEKIT_FAST_STROBE 0x5
#define RUMBLE_STAGEKIT_FASTEST_STROBE 0x6
#define RUMBLE_STAGEKIT_NO_STROBE 0x7
#define RUMBLE_STAGEKIT_ALLOFF 0xFF

// For these, left is a bitmask of leds (0-7) and right is the command again
#define RUMBLE_STAGEKIT_RED 0x80
#define RUMBLE_STAGEKIT_YELLOW 0x60
#define RUMBLE_STAGEKIT_GREEN 0x40
#define RUMBLE_STAGEKIT_BLUE 0x20
#define RUMBLE_STAGEKIT_OFF 0xFF

// set left to 0/1 for on and off, and right to these values for our commands
#define RUMBLE_SANTROLLER_OFF 0x0
#define RUMBLE_SANTROLLER_ON 0x1

#define RUMBLE_SANTROLLER_STAR_POWER_FILL 0x8
#define RUMBLE_SANTROLLER_STAR_POWER_ACTIVE 0x9
#define RUMBLE_SANTROLLER_MULTIPLIER 0xa
#define RUMBLE_SANTROLLER_SOLO 0xb
#define RUMBLE_SANTROLLER_NOTE_MISS 0x0C
#define RUMBLE_SANTROLLER_NOTE_HIT 0x90
#define RUMBLE_SANTROLLER_EUPHORIA_LED 0xA0


#define TUD_XINPUT_GAMEPAD_DESC_LEN    (9 + 17 + 7 + 7)

// XInput Gamepad Descriptor
#define TUD_XINPUT_GAMEPAD_DESCRIPTOR(_itfnum, _epin, _epout, _subtype) \
  /* Interface */\
  9, TUSB_DESC_INTERFACE, _itfnum, 0, 2, 0xFF, 0x5D, 0x01, 0,\
  /* XInput Gamepad descriptor */\
  17, 0x21, 0x10, 0x01, _subtype, 0x25, _epin, 0x14, 0x03, 0x03, 0x03, 0x04, 0x13, _epout, 0x08, 0x03, 0x03,\
  /* Endpoint In */\
  7, TUSB_DESC_ENDPOINT, _epin, TUSB_XFER_INTERRUPT, U16_TO_U8S_LE(0x20), 1,\
  /* Endpoint Out */\
  7, TUSB_DESC_ENDPOINT, _epout, TUSB_XFER_INTERRUPT, U16_TO_U8S_LE(0x20), 1

#define TUD_XINPUT_SECURITY_DESC_LEN    (9 + 6)

// XInput Gamepad Descriptor
#define TUD_XINPUT_SECURITY_DESCRIPTOR(_itfnum, _strnum) \
  /* Interface */\
  9, TUSB_DESC_INTERFACE, _itfnum, 0, 0, 0xFF, 0xFD, 0x13, _strnum,\
  /* XInput Gamepad descriptor */\
  0x06, 0x41, 0x00, 0x01, 0x01, 0x03\

#endif /* USB_DESCRIPTORS_H_ */