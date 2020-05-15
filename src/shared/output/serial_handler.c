#include "serial_handler.h"
#include "../input/input_direct.h"
#include "../input/input_ps2_cnt.h"
#include "../input/input_wii_ext.h"
#include "../util.h"
#include "usb/API.h"
#include <stdlib.h>

int buf_idx = 0;
int cmd = 0;
int size = 0;
int subcmd = 0;
static const uint8_t *pbuf = NULL;
static uint8_t *buf = NULL;
bool ext = false;
void get_info_buf(uint8_t data) {
  switch (data) {
  case INFO_VERSION:
    pbuf = (const uint8_t *)PSTR(VERSION);
    break;
  case INFO_SIGNATURE:
    pbuf = (const uint8_t *)PSTR(SIGNATURE);
    break;
  case INFO_MAIN_MCU:
    pbuf = (const uint8_t *)PSTR(MCU);
    break;
  case INFO_CPU_FREQ:
    pbuf = (const uint8_t *)PSTR(STR(F_CPU));
    break;
  case INFO_BOARD:
    pbuf = (const uint8_t *)PSTR(ARDWIINO_BOARD);
    break;
  case INFO_EXT:
    cmd = COMMAND_READ_CONFIG_VALUE;
    ext = true;
    if (config_pointer.main.input_type == WII) {
      buf = (uint8_t *)&wii_ext;
      size = 2;
    } else if (config_pointer.main.input_type == PS2) {
      buf = (uint8_t *)&ps2_type;
      size = 1;
    }
    return;
  }
  size = strlen_P((char *)pbuf);
}
void get_config_buf(uint8_t data) {
  size = 1;
  switch (data) {
  case CONFIG_INPUT_TYPE:
    buf = &config_pointer.main.input_type;
    break;
  case CONFIG_SUB_TYPE:
    buf = &config_pointer.main.sub_type;
    break;
  case CONFIG_TILT_TYPE:
    buf = &config_pointer.main.tilt_type;
    break;
  case CONFIG_MPU_6050_ORIENTATION:
    buf = &config_pointer.axis.mpu_6050_orientation;
    break;
  case CONFIG_TILT_SENSITIVITY:
    buf = (uint8_t *)&config_pointer.axis.tilt_sensitivity;
    break;
  case CONFIG_LED_TYPE:
    buf = &config_pointer.main.fret_mode;
    break;
  case CONFIG_MAP_JOY_DPAD:
    buf = (uint8_t *)&config_pointer.main.map_joy_to_dpad;
    break;
  case CONFIG_MAP_START_SEL_HOME:
    buf = (uint8_t *)&config_pointer.main.map_start_select_to_home;
    break;
  case CONFIG_MAP_ACCEL_RIGHT:
    buf = (uint8_t *)&config_pointer.main.map_accel_to_right;
    break;
  case CONFIG_PIN_UP:
    buf = &config_pointer.pins.up;
    break;
  case CONFIG_PIN_DOWN:
    buf = &config_pointer.pins.down;
    break;
  case CONFIG_PIN_LEFT:
    buf = &config_pointer.pins.left;
    break;
  case CONFIG_PIN_RIGHT:
    buf = &config_pointer.pins.right;
    break;
  case CONFIG_PIN_START:
    buf = &config_pointer.pins.start;
    break;
  case CONFIG_PIN_SELECT:
    buf = &config_pointer.pins.back;
    break;
  case CONFIG_PIN_LEFT_STICK:
    buf = &config_pointer.pins.left_stick;
    break;
  case CONFIG_PIN_RIGHT_STICK:
    buf = &config_pointer.pins.right_stick;
    break;
  case CONFIG_PIN_LB:
    buf = &config_pointer.pins.LB;
    break;
  case CONFIG_PIN_RB:
    buf = &config_pointer.pins.RB;
    break;
  case CONFIG_PIN_HOME:
    buf = &config_pointer.pins.home;
    break;
  case CONFIG_PIN_CAPTURE:
    buf = &config_pointer.pins.capture;
    break;
  case CONFIG_PIN_A:
    buf = &config_pointer.pins.a;
    break;
  case CONFIG_PIN_B:
    buf = &config_pointer.pins.b;
    break;
  case CONFIG_PIN_X:
    buf = &config_pointer.pins.x;
    break;
  case CONFIG_PIN_Y:
    buf = &config_pointer.pins.y;
    break;
  case CONFIG_PIN_LT:
    buf = &config_pointer.pins.lt.pin;
    break;
  case CONFIG_PIN_RT:
    buf = &config_pointer.pins.rt.pin;
    break;
  case CONFIG_PIN_L_X:
    buf = &config_pointer.pins.l_x.pin;
    break;
  case CONFIG_PIN_L_Y:
    buf = &config_pointer.pins.l_y.pin;
    break;
  case CONFIG_PIN_R_X:
    buf = &config_pointer.pins.r_x.pin;
    break;
  case CONFIG_PIN_R_Y:
    buf = &config_pointer.pins.r_y.pin;
    break;
  case CONFIG_KEY_UP:
    buf = &config_pointer.keys.up;
    break;
  case CONFIG_KEY_DOWN:
    buf = &config_pointer.keys.down;
    break;
  case CONFIG_KEY_LEFT:
    buf = &config_pointer.keys.left;
    break;
  case CONFIG_KEY_RIGHT:
    buf = &config_pointer.keys.right;
    break;
  case CONFIG_KEY_START:
    buf = &config_pointer.keys.start;
    break;
  case CONFIG_KEY_SELECT:
    buf = &config_pointer.keys.back;
    break;
  case CONFIG_KEY_LEFT_STICK:
    buf = &config_pointer.keys.left_stick;
    break;
  case CONFIG_KEY_RIGHT_STICK:
    buf = &config_pointer.keys.right_stick;
    break;
  case CONFIG_KEY_LB:
    buf = &config_pointer.keys.LB;
    break;
  case CONFIG_KEY_RB:
    buf = &config_pointer.keys.RB;
    break;
  case CONFIG_KEY_HOME:
    buf = &config_pointer.keys.home;
    break;
  case CONFIG_KEY_CAPTURE:
    buf = &config_pointer.keys.capture;
    break;
  case CONFIG_KEY_A:
    buf = &config_pointer.keys.a;
    break;
  case CONFIG_KEY_B:
    buf = &config_pointer.keys.b;
    break;
  case CONFIG_KEY_X:
    buf = &config_pointer.keys.x;
    break;
  case CONFIG_KEY_Y:
    buf = &config_pointer.keys.y;
    break;
  case CONFIG_KEY_LT:
    buf = &config_pointer.keys.lt;
    break;
  case CONFIG_KEY_RT:
    buf = &config_pointer.keys.rt;
    break;
  case CONFIG_KEY_L_X:
    size = 2;
    buf = &config_pointer.keys.l_x.neg;
    break;
  case CONFIG_KEY_L_Y:
    size = 2;
    buf = &config_pointer.keys.l_y.neg;
    break;
  case CONFIG_KEY_R_X:
    size = 2;
    buf = &config_pointer.keys.r_x.neg;
    break;
  case CONFIG_KEY_R_Y:
    size = 2;
    buf = &config_pointer.keys.r_y.neg;
    break;
  case CONFIG_AXIS_INVERT_LT:
    buf = (uint8_t *)&config_pointer.pins.lt.inverted;
    break;
  case CONFIG_AXIS_INVERT_RT:
    buf = (uint8_t *)&config_pointer.pins.rt.inverted;
    break;
  case CONFIG_AXIS_INVERT_L_X:
    buf = (uint8_t *)&config_pointer.pins.l_x.inverted;
    break;
  case CONFIG_AXIS_INVERT_L_Y:
    buf = (uint8_t *)&config_pointer.pins.l_y.inverted;
    break;
  case CONFIG_AXIS_INVERT_R_X:
    buf = (uint8_t *)&config_pointer.pins.r_x.inverted;
    break;
  case CONFIG_AXIS_INVERT_R_Y:
    buf = (uint8_t *)&config_pointer.pins.r_y.inverted;
    break;
  case CONFIG_THRESHOLD_JOY:
    buf = (uint8_t *)&config_pointer.axis.threshold_joy;
    break;
  case CONFIG_THRESHOLD_TRIGGER:
    buf = (uint8_t *)&config_pointer.axis.threshold_trigger;
    break;
  case CONFIG_THRESHOLD_DRUM:
    buf = (uint8_t *)&config_pointer.new_items.threshold_drums;
    break;
  case CONFIG_LED_COLOURS:
    size = sizeof(config_pointer.new_items.leds.colours);
    buf = (uint8_t *)&config_pointer.new_items.leds.colours;
    subcmd = data;
    size = 0;
    break;
  case CONFIG_LED_GH_COLOURS:
    size = sizeof(config_pointer.new_items.leds.ghColours);
    buf = (uint8_t *)&config_pointer.new_items.leds.ghColours;
    subcmd = data;
    size = 0;
    break;
  case CONFIG_LED_PINS:
    size = sizeof(config_pointer.new_items.leds.pins);
    buf = (uint8_t *)&config_pointer.new_items.leds.pins;
    subcmd = data;
    size = 0;
    break;
  case CONFIG_MIDI_CHANNEL:
    buf = (uint8_t *)&config_pointer.new_items.midi.channel;
    subcmd = data;
    size = 0;
    break;
  case CONFIG_MIDI_NOTE:
    buf = (uint8_t *)&config_pointer.new_items.midi.note;
    subcmd = data;
    size = 0;
    break;
  case CONFIG_MIDI_TYPE:
    buf = (uint8_t *)&config_pointer.new_items.midi.midi_type;
    subcmd = data;
    size = 0;
    break;
  }
}
void process_serial(uint8_t data) {
  if (cmd == 0) {
    cmd = data;
    size = 0;
    subcmd = 0;
    ext = false;
    switch (cmd) {
    case COMMAND_REBOOT:
      reboot();
      break;
    case COMMAND_JUMP_BOOTLOADER:
      bootloader();
      break;
    case COMMAND_SET_GH_LEDS:
      buf = controller.leds.leds;
      size = 5;
      return;
    case COMMAND_SET_LED_COLOUR:
      buf = (uint8_t *)&controller.leds.gui;
      size = 3;
      return;
    case COMMAND_FIND_DIGITAL:
      find_digital();
      cmd = 0;
      break;
    case COMMAND_FIND_ANALOG:
      find_analog();
      cmd = 0;
      break;
    case COMMAND_FIND_STOP:
      stop_searching();
      cmd = 0;
      break;
    case COMMAND_WRITE_CONFIG_VALUE:
    case COMMAND_READ_INFO:
    case COMMAND_READ_CONFIG_VALUE:
      return;
    default:
      cmd = 0;
    }
  } else if (size == 0) {
    if (subcmd) {
      switch (subcmd) {
      case CONFIG_LED_COLOURS:
        buf += data * 4;
        size = 4;
        break;
      case CONFIG_LED_GH_COLOURS:
        buf += data * 4;
        size = 4;
        break;
      case CONFIG_LED_PINS:
        buf += data;
        size = 1;
        break;
      case CONFIG_MIDI_CHANNEL:
        buf += data;
        size = 1;
        break;
      case CONFIG_MIDI_NOTE:
        buf += data;
        size = 1;
        break;
      case CONFIG_MIDI_TYPE:
        buf += data;
        size = 1;
        break;
      }
      if (cmd == COMMAND_WRITE_CONFIG_VALUE) { return; }
    } else {
      switch (cmd) {
      case COMMAND_WRITE_CONFIG_VALUE:
        get_config_buf(data);
        return;
      case COMMAND_READ_INFO:
        get_info_buf(data);
        break;
      case COMMAND_READ_CONFIG_VALUE:
        get_config_buf(data);
        if (subcmd) { return; }
        break;
      }
    }
    while (size) {
      if (cmd == COMMAND_READ_INFO) {
        write_usb(pgm_read_byte(pbuf++));
      } else if (ext){
        write_usb(*(buf++));
      } else {
        write_usb(eeprom_read_byte(buf++));
      }
      size--;
    }
  } else {
    eeprom_update_byte(buf++,data);
    size--;
  }
  if (size == 0) {
    // if (subcmd == CONFIG_LED_GH_COLOURS) { write_config(); }
    write_usb('\r');
    write_usb('\n');
    cmd = 0;
  }
}