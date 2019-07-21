#pragma once
#include <LUFA/Drivers/USB/USB.h>
#include <stdbool.h>
/** Type define for the gamepad HID report structure, for creating and sending
 * HID reports to the host PC. This mirrors the layout described to the host in
 * the HID report descriptor, in Descriptors.c.
 */
typedef struct {
  uint16_t buttons;
  uint8_t hat;

  // left and right analog sticks, 0x00 left/up, 0x80 middle, 0xff right/down

  uint8_t l_x;
  uint8_t l_y;
  uint8_t r_x;
  uint8_t r_y;

  // Gonna assume these are button analog values for the d-pad.
  // NOTE: NOT EVEN SURE THIS IS RIGHT, OR IN THE CORRECT ORDER
  uint8_t right_axis;
  uint8_t left_axis;
  uint8_t up_axis;
  uint8_t down_axis;

  // button axis, 0x00 = unpressed, 0xff = fully pressed

  uint8_t triangle_axis;
  uint8_t circle_axis;
  uint8_t cross_axis;
  uint8_t square_axis;

  uint8_t l1_axis;
  uint8_t r1_axis;
  uint8_t l2_axis;
  uint8_t r2_axis;
} USB_PS3Report_Data_t;
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
typedef union {
  USB_KeyboardReport_Data_t keyboard;
  USB_PS3Report_Data_t ps3;
  USB_XInputReport_Data_t xinput;
} previous_buffer_t;
