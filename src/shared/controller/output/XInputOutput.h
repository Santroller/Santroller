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

class XInputOutput : public Output {
public:
  void init();
  void update(Controller controller);
  bool ready();
  void usb_connect();
  void usb_disconnect();
  void usb_configuration_changed();
  void usb_control_request();
  void usb_start_of_frame();
  XInputOutput();

private:
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
  USB_JoystickReport_Data_t gamepad_state;
  bool isReady;
};