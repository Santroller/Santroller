#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
#include <stdint.h>

#include "defines.h"
#if __has_include("config_data.h")
#include "config_data.h"
#endif

// ConsoleType can change due to console detection
extern uint8_t consoleType;
extern uint8_t deviceType;
#ifdef CONFIGURABLE_BLOBS
extern const uint8_t* config;
extern const uint8_t* config_blobs;
#else
extern const uint8_t config[CONFIGURATION_LEN];
#endif
#ifdef __cplusplus
}
#endif
#include "defines.h"
#include "reports/controller_reports.h"
#define DEVICE_TYPE_IS_LIVE_GUITAR (deviceType == LIVE_GUITAR)
#define DEVICE_TYPE_IS_GAMEPAD (deviceType == GAMEPAD)
#define DEVICE_TYPE_IS_GUITAR (DEVICE_TYPE_IS_CONTROLLER && (deviceType == ROCK_BAND_GUITAR || deviceType == GUITAR_HERO_GUITAR))
#define DEVICE_TYPE_IS_DRUM (DEVICE_TYPE_IS_CONTROLLER && (deviceType == ROCK_BAND_DRUMS || deviceType == GUITAR_HERO_DRUMS))
#define DEVICE_TYPE_IS_INSTRUMENT (DEVICE_TYPE_IS_CONTROLLER && (DEVICE_TYPE_IS_GUITAR || deviceType == LIVE_GUITAR || DEVICE_TYPE_IS_DRUM || deviceType == DJ_HERO_TURNTABLE || deviceType == STAGE_KIT || DEVICE_TYPE_IS_PRO))
#define DEVICE_TYPE_IS_KEYBOARD (deviceType == KEYBOARD_MOUSE_TYPE)
#define DEVICE_TYPE_IS_CONTROLLER (!(DEVICE_TYPE_IS_KEYBOARD))
#define DEVICE_TYPE_IS_PRO (deviceType == ROCK_BAND_PRO_GUITAR_MUSTANG || deviceType == ROCK_BAND_PRO_GUITAR_SQUIRE || deviceType == ROCK_BAND_PRO_KEYS)

typedef union {
    USB_Host_Data_t gamepad;
    USB_NKRO_Data_t keyboard;
    USB_MIDI_Data_t midi;
    USB_Mouse_Data_t mouse;
} last_report_t;
extern const uint8_t announce_guitar[32];
extern const uint8_t xb1_descriptor_guitar[256];
extern const uint8_t xb1_descriptor_guitar_end[6];
extern const uint8_t announce_ghl[32];
extern const uint8_t xb1_descriptor_ghl[329];
extern const uint8_t xb1_descriptor_ghl_end[6];
extern const uint8_t announce_drums[32];
extern const uint8_t xb1_descriptor_drums[224];
extern const uint8_t xb1_descriptor_drums_end[6];
extern const uint8_t announce_gamepad[32];
extern const uint8_t xb1_descriptor_gamepad[244];
extern const uint8_t xb1_descriptor_gamepad_end[6];
#define SUPPORTS_MIDI DEVICE_TYPE == MIDI_TYPE
#define SUPPORTS_HID DEVICE_TYPE != MIDI_TYPE
#define SUPPORTS_PS4 (DEVICE_TYPE_IS_GAMEPAD || DEVICE_TYPE == LIVE_GUITAR || PS4_INSTRUMENT)
#define SUPPORTS_PICO defined(ARDUINO_ARCH_RP2040)
#define SUPPORTS_AVR defined(__AVR__)
#define SUPPORTS_TEENSY (defined(__arm__) && defined(CORE_TEENSY))
#define BLUETOOTH_RX (defined(BLUETOOTH_RX_BLE) || defined(BLUETOOTH_RX_CLASSIC))
#define BLUETOOTH (defined(BLUETOOTH_TX) || BLUETOOTH_RX)
enum hid_reports_t {
    REPORT_ID_NONE,
    REPORT_ID_NKRO,
    REPORT_ID_CONSUMER,
    REPORT_ID_MOUSE,
    REPORT_ID_END
};

typedef struct {
    union {
        USB_Host_Data_t host;
    } lastControllerReport;
    USB_6KRO_Data_t last6KROReport;
    USB_NKRO_Data_t lastNKROReport;
    USB_Mouse_Data_t lastMouseReport;
    USB_ConsumerControl_Data_t lastConsumerReport;
} USB_LastReport_Data_t;

#define USB_VERSION_BCD(Major, Minor, Revision) \
    (((Major & 0xFF) << 8) |                    \
     ((Minor & 0x0F) << 4) |                    \
     (Revision & 0x0F))

#ifdef CONFIGURABLE_BLOBS
inline int16_t read_int16(uint16_t idx) {
    return *((int16_t*)&config_blobs[idx]);
}
inline uint16_t read_uint16(uint16_t idx) {
    return *((uint16_t*)&config_blobs[idx]);
}
inline double read_double(uint16_t idx) {
    return *((double*)&config_blobs[idx]);
}
#endif

extern bool disable_multiplexer;

#if SUPPORTS_PICO
#ifdef TWI_1_OUTPUT
#define RXWIRE Wire1
#else
#define RXWIRE Wire
#endif
#else
#define RXWIRE Wire
#endif

