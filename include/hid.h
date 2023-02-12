#pragma once
#include <stdint.h>

#include "config.h"
#ifdef TICK_NKRO
#define NKRO_SIZE 65
#else
#define NKRO_SIZE 0
#endif
#ifdef TICK_CONSUMER
#define CONSUMER_SIZE 53
#else
#define CONSUMER_SIZE 0
#endif
#ifdef TICK_MOUSE
#define MOUSE_SIZE 71
#else
#define MOUSE_SIZE 0
#endif

// HID "Report IDs" used by xinput for 360 rumble and led
#define XBOX_LED_ID 0x01
#define XBOX_RUMBLE_ID 0x00
#define INTERRUPT_ID 0x16

// HID "Report IDs" used for rumble and led data from the console
#define PS3_LED_ID 0x01
#define XBOX_ONE_GHL_POKE_ID 0x02
#define XONE_IDENTIFY_ID 0x04
#define DJ_LED_ID 0x91
#define SANTROLLER_PS3_RUMBLE_ID 0x92

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

#define RUMBLE_SANTROLLER_COMBO_1 0x8
#define RUMBLE_SANTROLLER_COMBO_2 0x9
#define RUMBLE_SANTROLLER_COMBO_3 0xa
#define RUMBLE_SANTROLLER_COMBO_4 0xb
#define RUMBLE_SANTROLLER_SOLO 0xc
#define RUMBLE_SANTROLLER_OPEN_KICK 0xd
#define RUMBLE_SANTROLLER_FRET_GREEN 0xe
#define RUMBLE_SANTROLLER_FRET_RED 0xf
#define RUMBLE_SANTROLLER_FRET_YELLOW 0x10
#define RUMBLE_SANTROLLER_FRET_BLUE 0x11
#define RUMBLE_SANTROLLER_FRET_ORANGE 0x12

// set left to the star power value, and right to these values for our commands
// For compatibility reasons, probably start star power at 0x20, so that we never end up with issues on turntables
// Since then we would just check left == right for turntable state
#define RUMBLE_SANTROLLER_STAR_POWER_FILL 0x13
#define RUMBLE_SANTROLLER_STAR_POWER_ACTIVE 0x14
extern const uint8_t keyboard_mouse_descriptor[NKRO_SIZE + CONSUMER_SIZE + MOUSE_SIZE];
extern const uint8_t ps3_descriptor[137];
extern const uint8_t pc_descriptor[150];
void handle_auth_led(void);
void hid_set_report(const uint8_t *data, uint8_t len, uint8_t reportType, uint8_t report_id);
uint8_t hid_get_report(uint8_t *data, uint8_t reportType, uint8_t report_id);
