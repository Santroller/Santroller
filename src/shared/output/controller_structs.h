#pragma once
#define __INCLUDE_FROM_HID_DRIVER
#define __INCLUDE_FROM_USB_DRIVER
#define USB_DEVICE_ONLY
#define USB_CAN_BE_DEVICE
#include <LUFA/Drivers/USB/Class/Common/HIDClassCommon.h>
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

  uint8_t axis[12];
  //Accel values are 10 bits with padding.
  uint16_t accel[4];

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
} output_report_size_t;