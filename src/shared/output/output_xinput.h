#pragma once
#include <stdint.h>
#include <LUFA/Drivers/USB/USB.h>
#include "../controller/controller.h"
#include "../output_handler.h"
/* Enums */
#define XBOX_EPSIZE 32
/* Macros: */
/** Endpoint address of the Joystick HID reporting IN endpoint. */
#define JOYSTICK_EPADDR_IN (ENDPOINT_DIR_IN | 1)

typedef struct {
  USB_Descriptor_Header_t Header;
  uint8_t data[2];
  uint8_t subtype;
  uint8_t data2[15];
} USB_HID_XBOX_Descriptor_HID_t;

typedef struct {
  USB_Descriptor_Configuration_Header_t Config;
  USB_Descriptor_Interface_t Interface0;
  USB_HID_XBOX_Descriptor_HID_t XInputUnknown;
  USB_Descriptor_Endpoint_t DataInEndpoint0;
  USB_Descriptor_Endpoint_t DataOutEndpoint0;
} Xinput_Descriptor_Configuration_t;

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
void xinput_init(event_pointers*);
void xinput_tick(controller_t);