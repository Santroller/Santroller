#pragma once

#include <stdint.h>
#define USB_PID 0x2882

#define USB_VID 0x1209
#define USB_VERSION_BCD(Major, Minor, Revision) \
    (((Major & 0xFF) << 8) |                    \
     ((Minor & 0x0F) << 4) |                    \
     (Revision & 0x0F))

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
    OS_COMPATIBLE_SECTION CompatID[];
} __attribute__((packed)) OS_COMPATIBLE_ID_DESCRIPTOR;

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
enum
{
  ITF_NUM_HID,
  // ITF_NUM_XONE=1,
  ITF_NUM_XINPUT,
  ITF_NUM_XINPUT2,
  ITF_NUM_XINPUT3,
  ITF_NUM_XINPUT4,
  ITF_NUM_XINPUT_SECURITY,
//   ITF_NUM_XINPUT_AUDIO,
//   ITF_NUM_XINPUT_PLUGIN_MODULE,
  // ITF_NUM_OGXBOX,
  ITF_NUM_TOTAL
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


enum
{
  STRID_LANGID = 0,
  STRID_MANUFACTURER,
  STRID_PRODUCT,
  STRID_SERIAL,
  STRID_XSM3,
  STRID_MSFT,
  STRID_GHA_LED,
  STRID_GHA_INPUT,
  STRID_PG_GUITAR_PRODUCT,
  STRID_PG_DRUM_PRODUCT
};


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

// XInput Audio Descriptor
#define TUD_XINPUT_AUDIO_DESCRIPTOR(_itfnum, _epin, _epout, _epin2, _epout2) \
  /* Interface */\
  9, TUSB_DESC_INTERFACE, _itfnum, 0, 2, 0xFF, 0x5D, 0x03, 0,\
  /* XInput Audio descriptor */\
  27, 0x21, 0x00, 0x01, 0x01, 0x01, _epin, 0x40, 0x01, _epout, 0x20, 0x16, _epin2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16, _epout2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
  /* Endpoint In (mic) */\
  7, TUSB_DESC_ENDPOINT, _epin, TUSB_XFER_INTERRUPT, U16_TO_U8S_LE(0x20), 2,\
  /* Endpoint Out (audio out)*/\
  7, TUSB_DESC_ENDPOINT, _epout, TUSB_XFER_INTERRUPT, U16_TO_U8S_LE(0x20), 4,\
  /* Endpoint In 2 (unk 1)*/\
  7, TUSB_DESC_ENDPOINT, _epin2, TUSB_XFER_INTERRUPT, U16_TO_U8S_LE(0x20), 64,\
  /* Endpoint Out 2 (unk 2)*/\
  7, TUSB_DESC_ENDPOINT, _epout2, TUSB_XFER_INTERRUPT, U16_TO_U8S_LE(0x20), 16

// XInput Plugin Module Descriptor
#define TUD_XINPUT_PLUGIN_MODULE_DESCRIPTOR(_itfnum, _epin) \
  /* Interface */\
  9, TUSB_DESC_INTERFACE, _itfnum, 0, 1, 0xFF, 0x5D, 0x02, 0,\
  /* XInput plugin module descriptor */\
  9, 0x21, 0x00, 0x01, 0x01, 0x22, _epin, 0x07, 0x00,\
  /* Endpoint In */\
  7, TUSB_DESC_ENDPOINT, _epin, TUSB_XFER_INTERRUPT, U16_TO_U8S_LE(0x20), 16

#define TUD_XINPUT_SECURITY_DESC_LEN    (9 + 6)

// XInput Security Descriptor
#define TUD_XINPUT_SECURITY_DESCRIPTOR(_itfnum, _strnum) \
  /* Interface */\
  9, TUSB_DESC_INTERFACE, _itfnum, 0, 0, 0xFF, 0xFD, 0x13, _strnum,\
  /* XInput Security descriptor */\
  6, 0x41, 0x00, 0x01, 0x01, 0x03

// XInput Gamepad Descriptor
#define TUD_OGXBOX_GAMEPAD_DESCRIPTOR(_itfnum, _epin, _epout) \
  /* Interface */\
  9, TUSB_DESC_INTERFACE, _itfnum, 0, 2, 0x58, 0x42, 0x00, 0,\
  /* Endpoint In */\
  7, TUSB_DESC_ENDPOINT, _epin, TUSB_XFER_INTERRUPT, U16_TO_U8S_LE(0x20), 4,\
  /* Endpoint Out */\
  7, TUSB_DESC_ENDPOINT, _epout, TUSB_XFER_INTERRUPT, U16_TO_U8S_LE(0x20), 4

// XOne Gamepad Descriptor
#define TUD_XONE_GAMEPAD_DESCRIPTOR(_itfnum, _epin, _epout) \
  /* Interface */\
  9, TUSB_DESC_INTERFACE, _itfnum, 0, 2, 0xff, 0x47, 0xd0, 0,\
  /* Endpoint In */\
  7, TUSB_DESC_ENDPOINT, _epin, TUSB_XFER_INTERRUPT, U16_TO_U8S_LE(0x40), 1,\
  /* Endpoint Out */\
  7, TUSB_DESC_ENDPOINT, _epout, TUSB_XFER_INTERRUPT, U16_TO_U8S_LE(0x40), 1

// GHArcade Gamepad Descriptor
#define TUD_GHARCADE_VENDOR_DESCRIPTOR(_itfnum, _epin1, _epout, _epin2, _strnum) \
  /* Interface */\
  9, TUSB_DESC_INTERFACE, _itfnum, 0, 3, 0xff, 0x01, 0xFF, _strnum,\
  /* Endpoint In */\
  7, TUSB_DESC_ENDPOINT, _epin1, TUSB_XFER_INTERRUPT, U16_TO_U8S_LE(0x40), 1,\
  /* Endpoint Out */\
  7, TUSB_DESC_ENDPOINT, _epout, TUSB_XFER_INTERRUPT, U16_TO_U8S_LE(0x40), 1, \
  /* Endpoint In */\
  7, TUSB_DESC_ENDPOINT, _epin2, TUSB_XFER_INTERRUPT, U16_TO_U8S_LE(0x40), 1

