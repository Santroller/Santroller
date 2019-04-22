#pragma once
/* Includes: */
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <string.h>

#include "../../../config/config.h"
extern "C" {
#include "../lufa/Descriptors.h"
}
#include "Output.h"
#include "stdint.h"
#include "../lufa/wcid.h"

#define XBOX_DPAD_UP 0
#define XBOX_DPAD_DOWN 1
#define XBOX_DPAD_LEFT 2
#define XBOX_DPAD_RIGHT 3
#define XBOX_START 4
#define XBOX_BACK 5
#define XBOX_LEFT_STICK 6
#define XBOX_RIGHT_STICK 7

// digital_buttons_2
#define XBOX_LB 0
#define XBOX_RB 1
#define XBOX_HOME 2
#define XBOX_A 4
#define XBOX_B 5
#define XBOX_X 6
#define XBOX_Y 7

/* Enums */
#define XBOX_EPSIZE 32
/* Macros: */
/** Endpoint address of the Joystick HID reporting IN endpoint. */
#define JOYSTICK_EPADDR_IN (ENDPOINT_DIR_IN | 1)

typedef struct {
  USB_Descriptor_Header_t Header;
  uint8_t data[15];
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
} USB_JoystickReport_Data_t;