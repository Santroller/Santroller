#pragma once
/* Includes: */
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <string.h>

#include "../../config/eeprom.h"
extern "C" {
#include "../lufa/Descriptors.h"
}
#include "../lufa/wcid.h"
#include "Output.h"
#include "stdint.h"


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

class XInputOutput : public Output {
public:
  Xinput_Descriptor_Configuration_t ConfigurationDescriptor;
  void init();
  void update(Controller controller);
  void usb_connect();
  void usb_disconnect();
  void usb_configuration_changed();
  void usb_control_request();
  void usb_start_of_frame();
  uint16_t get_descriptor(const uint8_t DescriptorType,
                          const uint8_t DescriptorNumber,
                          const void **const DescriptorAddress,
                          uint8_t *const DescriptorMemorySpace);
};