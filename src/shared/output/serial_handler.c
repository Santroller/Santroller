#include "serial_handler.h"
#include "../input/input_wii_ext.h"
#include "usb/API.h"
#include <stdlib.h>
#include "../input/input_direct.h"

extern uint16_t id;
config_t new_config;
void init_serial(void) { new_config = config; }
int buf_idx = 0;
int cmd = 0;
int size = 0;
static uint8_t *buf = NULL;
uint8_t str[50];
void get_info_buf(uint8_t data) {
  switch (data) {
  case INFO_VERSION:
    buf = (uint8_t *)version;
    break;
  case INFO_SIGNATURE:
    buf = (uint8_t *)signature;
    break;
  case INFO_MAIN_MCU:
    buf = (uint8_t *)mcu;
    break;
  case INFO_USB_MCU:
    buf = (uint8_t *)usb_mcu;
    break;
  case INFO_CPU_FREQ:
    buf = (uint8_t *)freq;
    break;
  case INFO_USB_CPU_FREQ:
    buf = (uint8_t *)usb_freq;
    break;
  case INFO_BOARD:
    buf = (uint8_t *)board;
    break;
  case INFO_WII_EXT:
    get_wii_device_name((char *)str);
    buf = str;
    break;
  }
  size = strlen((char *)buf);
}
void get_config_buf(uint8_t data) {
  size = 1;
  switch (data) {
  case CONFIG_CURRENT_INPUT_TYPE:
    buf = &config.main.input_type;
    break;
  case CONFIG_CURRENT_SUB_TYPE:
    buf = &config.main.sub_type;
    break;
  case CONFIG_INPUT_TYPE:
    buf = &new_config.main.input_type;
    break;
  case CONFIG_SUB_TYPE:
    buf = &new_config.main.sub_type;
    break;
  case CONFIG_TILT_TYPE:
    buf = &new_config.main.tilt_type;
    break;
  case CONFIG_MPU_6050_ORIENTATION:
    buf = &new_config.axis.mpu_6050_orientation;
    break;
  case CONFIG_TILT_SENSITIVITY:
    buf = (uint8_t *)&new_config.axis.tilt_sensitivity;
    break;
  case CONFIG_LED_TYPE:
    buf = &new_config.main.fret_mode;
    break;
  case CONFIG_MAP_JOY_DPAD:
    buf = (uint8_t *)&new_config.main.map_joy_to_dpad;
    break;
  case CONFIG_MAP_START_SEL_HOME:
    buf = (uint8_t *)&new_config.main.map_start_select_to_home;
    break;
  case CONFIG_MAP_ACCEL_RIGHT:
    buf = (uint8_t *)&new_config.main.map_accel_to_right;
    break;
  case CONFIG_PIN_UP:
    buf = &new_config.pins.up;
    break;
  case CONFIG_PIN_DOWN:
    buf = &new_config.pins.down;
    break;
  case CONFIG_PIN_LEFT:
    buf = &new_config.pins.left;
    break;
  case CONFIG_PIN_RIGHT:
    buf = &new_config.pins.right;
    break;
  case CONFIG_PIN_START:
    buf = &new_config.pins.start;
    break;
  case CONFIG_PIN_SELECT:
    buf = &new_config.pins.back;
    break;
  case CONFIG_PIN_LEFT_STICK:
    buf = &new_config.pins.left_stick;
    break;
  case CONFIG_PIN_RIGHT_STICK:
    buf = &new_config.pins.right_stick;
    break;
  case CONFIG_PIN_LB:
    buf = &new_config.pins.LB;
    break;
  case CONFIG_PIN_RB:
    buf = &new_config.pins.RB;
    break;
  case CONFIG_PIN_HOME:
    buf = &new_config.pins.home;
    break;
  case CONFIG_PIN_CAPTURE:
    buf = &new_config.pins.capture;
    break;
  case CONFIG_PIN_A:
    buf = &new_config.pins.a;
    break;
  case CONFIG_PIN_B:
    buf = &new_config.pins.b;
    break;
  case CONFIG_PIN_X:
    buf = &new_config.pins.x;
    break;
  case CONFIG_PIN_Y:
    buf = &new_config.pins.y;
    break;
  case CONFIG_PIN_LT:
    buf = &new_config.pins.lt.pin;
    break;
  case CONFIG_PIN_RT:
    buf = &new_config.pins.rt.pin;
    break;
  case CONFIG_PIN_L_X:
    buf = &new_config.pins.l_x.pin;
    break;
  case CONFIG_PIN_L_Y:
    buf = &new_config.pins.l_y.pin;
    break;
  case CONFIG_PIN_R_X:
    buf = &new_config.pins.r_x.pin;
    break;
  case CONFIG_PIN_R_Y:
    buf = &new_config.pins.r_y.pin;
    break;
  case CONFIG_KEY_UP:
    buf = &new_config.keys.up;
    break;
  case CONFIG_KEY_DOWN:
    buf = &new_config.keys.down;
    break;
  case CONFIG_KEY_LEFT:
    buf = &new_config.keys.left;
    break;
  case CONFIG_KEY_RIGHT:
    buf = &new_config.keys.right;
    break;
  case CONFIG_KEY_START:
    buf = &new_config.keys.start;
    break;
  case CONFIG_KEY_SELECT:
    buf = &new_config.keys.back;
    break;
  case CONFIG_KEY_LEFT_STICK:
    buf = &new_config.keys.left_stick;
    break;
  case CONFIG_KEY_RIGHT_STICK:
    buf = &new_config.keys.right_stick;
    break;
  case CONFIG_KEY_LB:
    buf = &new_config.keys.LB;
    break;
  case CONFIG_KEY_RB:
    buf = &new_config.keys.RB;
    break;
  case CONFIG_KEY_HOME:
    buf = &new_config.keys.home;
    break;
  case CONFIG_KEY_CAPTURE:
    buf = &new_config.keys.capture;
    break;
  case CONFIG_KEY_A:
    buf = &new_config.keys.a;
    break;
  case CONFIG_KEY_B:
    buf = &new_config.keys.b;
    break;
  case CONFIG_KEY_X:
    buf = &new_config.keys.x;
    break;
  case CONFIG_KEY_Y:
    buf = &new_config.keys.y;
    break;
  case CONFIG_KEY_LT:
    buf = &new_config.keys.lt;
    break;
  case CONFIG_KEY_RT:
    buf = &new_config.keys.rt;
    break;
  case CONFIG_KEY_L_X:
    size = 2;
    buf = &new_config.keys.l_x.neg;
    break;
  case CONFIG_KEY_L_Y:
    size = 2;
    buf = &new_config.keys.l_y.neg;
    break;
  case CONFIG_KEY_R_X:
    size = 2;
    buf = &new_config.keys.r_x.neg;
    break;
  case CONFIG_KEY_R_Y:
    size = 2;
    buf = &new_config.keys.r_y.neg;
    break;
  case CONFIG_AXIS_INVERT_LT:
    buf = (uint8_t *)&new_config.pins.lt.inverted;
    break;
  case CONFIG_AXIS_INVERT_RT:
    buf = (uint8_t *)&new_config.pins.rt.inverted;
    break;
  case CONFIG_AXIS_INVERT_L_X:
    buf = (uint8_t *)&new_config.pins.l_x.inverted;
    break;
  case CONFIG_AXIS_INVERT_L_Y:
    buf = (uint8_t *)&new_config.pins.l_y.inverted;
    break;
  case CONFIG_AXIS_INVERT_R_X:
    buf = (uint8_t *)&new_config.pins.r_x.inverted;
    break;
  case CONFIG_AXIS_INVERT_R_Y:
    buf = (uint8_t *)&new_config.pins.r_y.inverted;
    break;
  case CONFIG_THRESHOLD_JOY:
    buf = (uint8_t *)&new_config.axis.threshold_joy;
    break;
  case CONFIG_THRESHOLD_TRIGGER:
    buf = (uint8_t *)&new_config.axis.threshold_trigger;
    break;
  }
}
void process_serial(uint8_t data) {
  if (cmd == 0) {
    cmd = data;
    size = 0;
    switch (cmd) {
    case COMMAND_START_CONFIG:
      new_config = config;
      write_usb('\r');
      write_usb('\n');
      cmd = 0;
      break;
    case COMMAND_APPLY_CONFIG:
      config = new_config;
      write_config();
      reboot();
      break;
    case COMMAND_JUMP_BOOTLOADER:
      bootloader();
      break;
    case COMMAND_SET_LEDS:
      buf = controller.leds.leds;
      size = 5;
      break;
    case COMMAND_SET_LED_GUI:
      buf = (uint8_t*)&controller.leds.gui;
      size = 4;
      break;
    case COMMAND_FIND_DIGITAL:
      write_usb(find_digital());
      break;
    case COMMAND_FIND_ANALOG:
      write_usb(find_analog());
      break;
    case COMMAND_WRITE_CONFIG_VALUE:
    case COMMAND_READ_INFO:
    case COMMAND_READ_CONFIG_VALUE:
      break;
    default:
      cmd = 0;
    }
    return;
  } else if (size == 0) {
    switch (cmd) {
    case COMMAND_WRITE_CONFIG_VALUE:
      get_config_buf(data);
      return;
    case COMMAND_READ_INFO:
      get_info_buf(data);
      break;
    case COMMAND_READ_CONFIG_VALUE:
      get_config_buf(data);
      break;
    }
    while (size) {
      write_usb(*(buf++));
      size--;
    }
  } else {
    *(buf++) = data;
    size--;
  }
  if (size == 0) {
    write_usb('\r');
    write_usb('\n');
    cmd = 0;
  }
}