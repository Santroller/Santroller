#pragma once

#include "common/tusb_common.h"
#include "device/usbd.h"
#include "gamepad_device.h"
#include "protocols/xinput.hpp"
#include "device.hpp"
#include "enums.pb.h"
#include "config.hpp"
#include "usb/usb_descriptors.h"

#ifndef CFG_TUD_XINPUT_EPSIZE
#define CFG_TUD_XINPUT_EPSIZE 64
#endif
#define SERIAL_NUMBER_WVALUE 0x0000
#define INPUT_CAPABILITIES_WVALUE 0x0100
#define VIBRATION_CAPABILITIES_WVALUE 0x0000
#define XINPUT_DESC_TYPE_RESERVED 0x21
#define XINPUT_SECURITY_DESC_TYPE_RESERVED 0x41
#define XBOX_LED_ID 0x01
#define XBOX_RUMBLE_ID 0x00
typedef struct
{
    uint8_t bLength;         // Length of this descriptor.
    uint8_t bDescriptorType; // XINPUT_DESC_TYPE_RESERVED
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
typedef struct
{
    uint8_t bLength;         // Length of this descriptor.
    uint8_t bDescriptorType; // XINPUT_SECURITY_DESC_TYPE_RESERVED
    uint8_t reserved[4];
} __attribute__((packed)) XBOX_SECURITY_DESCRIPTOR;

enum
{
    XINPUT_GAMEPAD = 1,
    XINPUT_WHEEL = 2,
    XINPUT_ARCADE_STICK = 3,
    XINPUT_FLIGHT_STICK = 4,
    XINPUT_DANCE_PAD = 5,
    XINPUT_GUITAR = 6,
    XINPUT_GUITAR_ALTERNATE = 7,
    XINPUT_DRUMS = 8,
    XINPUT_STAGE_KIT = 9,
    XINPUT_GUITAR_BASS = 11,
    XINPUT_PRO_KEYS = 15,
    XINPUT_ARCADE_PAD = 19,
    XINPUT_TURNTABLE = 23,
    XINPUT_PRO_GUITAR = 25,
    XINPUT_DISNEY_INFINITY_AND_LEGO_DIMENSIONS = 33,
    XINPUT_SKYLANDERS = 36
};

typedef enum
{
    Controller,
    Security,
    Chatpad,
    Audio
} XInputDeviceType;

inline uint8_t get_subtype(void)
{
    switch (current_type)
    {
    case Gamepad:
    case PopNMusic:
    case ProjectDiva:
    case DJMax:
        return XINPUT_GAMEPAD;
    case Dancepad:
        return XINPUT_DANCE_PAD;
    case GuitarFreaks:
    case LiveGuitar:
    case GuitarHeroGuitar:
        return XINPUT_GUITAR_ALTERNATE;
    case RockBandGuitar:
        return XINPUT_GUITAR;
    case GuitarHeroDrums:
    case RockBandDrums:
        return XINPUT_DRUMS;
    case DjHeroTurntable:
        return XINPUT_TURNTABLE;
    case ProGuitarMustang:
    case ProGuitarSquire:
        return XINPUT_PRO_GUITAR;
    case ProKeys:
        return XINPUT_PRO_KEYS;
    case Taiko:
        return XINPUT_GAMEPAD;
    case StageKit:
        return XINPUT_STAGE_KIT;
    case KeyboardMouse:
        return XINPUT_GAMEPAD;
    case Wheel:
        return XINPUT_WHEEL;
    case DisneyInfinity:
    case LegoDimensions:
        return XINPUT_DISNEY_INFINITY_AND_LEGO_DIMENSIONS;
    case Skylanders:
        return XINPUT_SKYLANDERS;
    case FightStick:
        return XINPUT_ARCADE_STICK;
    case FlightStick:
        return XINPUT_FLIGHT_STICK;
    }
    return 1;
}

#define XINPUT_FLAGS_NO_NAV 0xFFFF
#define XINPUT_FLAGS_NONE 0x0000
#define XINPUT_FLAGS_FORCE_FEEDBACK 0x000D
#ifdef __cplusplus
extern "C"
{
#endif
    // Check if the interface is ready to use
    bool tud_xinput_n_ready(uint8_t itf);

    // Send report to host
    bool tud_xinput_n_report(uint8_t itf, void const *report, uint8_t len);
    void xinputd_init(void);
    void xinputd_reset(uint8_t rhport);
    uint16_t xinputd_open(uint8_t rhport, tusb_desc_interface_t const *itf_desc,
                          uint16_t max_len);
    bool xinputd_control_request(uint8_t rhport,
                                 tusb_control_request_t const *request);
    bool xinputd_control_complete(uint8_t rhport,
                                  tusb_control_request_t const *request);
    bool xinputd_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t event,
                         uint32_t xferred_bytes);
    bool xinputd_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request);

#ifdef __cplusplus
}
#endif

class XInputDevice : public UsbDevice
{
public:
    XInputDevice();
    void initialize();
    void process();
    OS_COMPATIBLE_SECTION get_compatible_section();
};