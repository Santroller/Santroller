#pragma once
#include "../controller/controller.h"
#include "../output_handler.h"
#include <LUFA/Drivers/USB/USB.h>
#include <stdint.h>
#include "usb/Descriptors.h"
/* Enums */
#define XBOX_EPSIZE 32

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
void xinput_init(event_pointers *, USB_ClassInfo_HID_Device_t *);
void xinput_tick(controller_t);