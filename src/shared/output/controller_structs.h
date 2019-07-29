#pragma once
#include <LUFA/Drivers/USB/USB.h>
#include <stdbool.h>
/** Type define for the gamepad HID report structure, for creating and sending
 * HID reports to the host PC. This mirrors the layout described to the host in
 * the HID report descriptor, in Descriptors.c.
 */
typedef struct {
  uint16_t buttons; //0
  uint8_t hat; //2

  // left and right analog sticks, 0x00 left/up, 0x80 middle, 0xff right/down

  uint8_t l_x; //3
  uint8_t l_y; //4
  uint8_t r_x; //5
  uint8_t r_y; //6

  uint8_t axis[12]; //7
  uint8_t accel[7]; //19

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
