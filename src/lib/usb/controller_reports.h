#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "attributes.h"
#include "midi_descriptors.h"

#define SIMULTANEOUS_KEYS 6
#define SIMULTANEOUS_MIDI 12
/** Type define for the gamepad HID report structure, for creating and sending
 * HID reports to the host PC. This mirrors the layout described to the host in
 * the HID report descriptor, in Descriptors.c.
 */
typedef struct {
  uint8_t rid;
  uint16_t buttons;
  uint8_t hat;

  // left and right analog sticks, 0x00 left/up, 0x80 middle, 0xff right/down

  uint8_t l_x;
  uint8_t l_y;
  uint8_t r_x;
  uint8_t r_y;

  uint8_t axis[12];
  // Accel values are 10 bits with padding.
  int16_t accel[4];

} USB_PS3Report_Data_t;

typedef struct {
  uint16_t buttons;
  uint8_t lt;
  uint8_t rt;
  int16_t l_x;
  int16_t l_y;
  int16_t r_x;
  int16_t r_y;
} XInput_Data_t;

typedef struct {
  uint8_t rid;
  uint8_t rsize;
  uint16_t buttons;
  uint8_t lt;
  uint8_t rt;
  int16_t l_x;
  int16_t l_y;
  int16_t r_x;
  int16_t r_y;
  uint8_t reserved_1[6];
} USB_XInputReport_Data_t;

typedef struct {
  uint8_t rid;
  uint8_t rsize;
  uint16_t buttons;
  uint8_t triggers[2];
  int16_t joys[4];
  uint8_t reserved_1[6];
} USB_XInputReport_Arr_Data_t;

typedef struct {
  TMIDI_EventPacket_t midi[SIMULTANEOUS_MIDI];
} USB_MIDI_Data_t;

typedef struct {
  uint8_t rid;
  uint8_t
      Button; /**< Button mask for currently pressed buttons in the mouse. */
  int8_t X;   /**< Current delta X movement of the mouse. */
  int8_t Y;   /**< Current delta Y movement on the mouse. */
  int8_t ScrollY; /** Current scroll Y delta movement on the mouse */
  int8_t ScrollX; /** Current scroll X delta movement on the mouse */
} TATTR_PACKED USB_MouseReport_Data_t;

typedef struct {
  uint8_t rid;
  uint8_t
      Modifier;     /**< Keyboard modifier byte, indicating pressed modifier
                     * keys (a combination of   \c HID_KEYBOARD_MODIFER_* masks).
                     */
  uint8_t Reserved; /**< Reserved for OEM use, always set to 0. */
  uint8_t KeyCode[SIMULTANEOUS_KEYS]; /**< Key codes of the currently pressed
                                         keys. */
} TATTR_PACKED USB_KeyboardReport_Data_t;


typedef enum {
    OFF, ALL_BLINKING, ONE_FLASH, TWO_FLASH, THREE_FLASH, FOUR_FLASH, ONE, TWO, THREE, FOUR, ROTATING, BLINKING_RET, SLOW_BLINKING_RET, ALTERNATING_RET
} xinput_led_t;
typedef struct {
    uint8_t id;
    uint8_t size;
    xinput_led_t led;
} TATTR_PACKED XInputLEDReport_t;

typedef struct {
    uint8_t id;
    uint8_t size;
    uint8_t unused;
    uint8_t leftRumble;
    uint8_t rightRumble;
    uint8_t unused2[3];
} TATTR_PACKED XInputRumbleReport_t;

typedef struct {
    uint8_t padding;
	uint8_t right_duration; /* Right motor duration (0xff means forever) */
	uint8_t right_motor_on; /* Right (small) motor on/off, only supports values of 0 or 1 (off/on) */
	uint8_t left_duration;    /* Left motor duration (0xff means forever) */
	uint8_t left_motor_force; /* left (large) motor, supports force values from 0 to 255 */
} TATTR_PACKED ps3_rumble_t;

typedef struct  {
	uint8_t time_enabled; /* the total time the led is active (0xff means forever) */
	uint8_t duty_length;  /* how long a cycle is in deciseconds (0 means "really fast") */
	uint8_t enabled;
	uint8_t duty_off; /* % of duty_length the led is off (0xff means 100%) */
	uint8_t duty_on;  /* % of duty_length the led is on (0xff mean 100%) */
} TATTR_PACKED ps3_led_t;

typedef struct {
	uint8_t report_id;
	ps3_rumble_t rumble;
	uint8_t padding[4];
	uint8_t leds_bitmap; /* bitmap of enabled LEDs: LED_1 = 0x02, LED_2 = 0x04, ... */
	ps3_led_t led[4];    /* LEDx at (4 - x) */
	ps3_led_t _reserved; /* LED5, not actually soldered */
} TATTR_PACKED ps3_output_report;


typedef union {
  USB_KeyboardReport_Data_t keyboard;
  USB_PS3Report_Data_t ps3;
  USB_XInputReport_Data_t xinput;
  USB_MIDI_Data_t midi;
  USB_MouseReport_Data_t mouse;
} USB_Report_Data_t;