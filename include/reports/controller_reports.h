#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "reports/ps3_reports.h"
#include "reports/xbox_one_reports.h"
#include "reports/xinput_reports.h"

#define SIMULTANEOUS_KEYS 6
#define SIMULTANEOUS_MIDI 12

typedef struct {
    // TMIDI_EventPacket_t midi[SIMULTANEOUS_MIDI];
} USB_MIDI_Data_t;

// typedef struct {
//   uint8_t rid;
//   uint8_t
//       Button; /**< Button mask for currently pressed buttons in the mouse. */
//   int8_t X;   /**< Current delta X movement of the mouse. */
//   int8_t Y;   /**< Current delta Y movement on the mouse. */
//   int8_t ScrollY; /** Current scroll Y delta movement on the mouse */
//   int8_t ScrollX; /** Current scroll X delta movement on the mouse */
// } __attribute__((packed)) USB_MouseReport_Data_t;

// typedef struct {
//   uint8_t rid;
//   uint8_t
//       Modifier;     /**< Keyboard modifier byte, indicating pressed modifier
//                      * keys (a combination of   \c HID_KEYBOARD_MODIFER_* masks).
//                      */
//   uint8_t Reserved; /**< Reserved for OEM use, always set to 0. */
//   uint8_t KeyCode[SIMULTANEOUS_KEYS]; /**< Key codes of the currently pressed
//                                          keys. */
// } __attribute__((packed)) USB_KeyboardReport_Data_t;

typedef union {
    // USB_KeyboardReport_Data_t keyboard;
    USB_MIDI_Data_t midi;
    uint8_t raw[64];
    // USB_MouseReport_Data_t mouse;
} USB_Report_Data_t;
