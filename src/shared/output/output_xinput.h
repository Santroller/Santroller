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
  uint8_t digital_buttons_1;
  uint8_t digital_buttons_2;
  uint8_t lt;
  uint8_t rt;
  int l_x;
  int l_y;
  int r_x;
  int r_y;
  uint8_t reserved_1[6];
} USB_XInputReport_Data_t;
void xinput_init(event_pointers *);
void xinput_tick(controller_t);