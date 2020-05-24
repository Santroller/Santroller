#include "serial_handler.h"
#include "input/inputs/direct.h"
#include "input/inputs/ps2_cnt.h"
#include "input/inputs/wii_ext.h"
#include "serial_commands.h"
#include "util/util.h"
#include <stdlib.h>

int currentCommand = 0;
int currentCommandSize = 0;
int currentWrite = 0;
static const uint8_t *constDataToRead = NULL;
static uint8_t *dataToReadWrite = NULL;
bool dataInRam = false;
bool reading = true;
bool hasExtraData = false;
bool started = false;
void get_info_buf(uint8_t data) {
  switch (data) {
  case INFO_VERSION:
    constDataToRead = (const uint8_t *)PSTR(VERSION);
    break;
  case INFO_SIGNATURE:
    constDataToRead = (const uint8_t *)PSTR(SIGNATURE);
    break;
  case INFO_MAIN_MCU:
    constDataToRead = (const uint8_t *)PSTR(MCU);
    break;
  case INFO_CPU_FREQ:
    constDataToRead = (const uint8_t *)PSTR(STR(F_CPU));
    break;
  case INFO_BOARD:
    constDataToRead = (const uint8_t *)PSTR(ARDWIINO_BOARD);
    break;
  case INFO_EXT:
    dataInRam = true;
    if (config.main.inputType == WII) {
      dataToReadWrite = (uint8_t *)&wiiExtensionID;
      currentCommandSize = 2;
    } else if (config.main.inputType == PS2) {
      dataToReadWrite = (uint8_t *)&ps2CtrlType;
      currentCommandSize = 1;
    }
    return;
  }
  currentCommandSize = strlen_P((char *)constDataToRead);
}
void get_config_buf(uint8_t data) {
  currentCommandSize = 1;
  switch (data) {
  case CONFIG_INPUT_TYPE:
    dataToReadWrite = &config_pointer.main.inputType;
    break;
  case CONFIG_SUB_TYPE:
    dataToReadWrite = &config_pointer.main.subType;
    break;
  case CONFIG_TILT_TYPE:
    dataToReadWrite = &config_pointer.main.tiltType;
    break;
  case CONFIG_MPU_6050_ORIENTATION:
    dataToReadWrite = &config_pointer.axis.mpu6050Orientation;
    break;
  case CONFIG_TILT_SENSITIVITY:
    dataToReadWrite = (uint8_t *)&config_pointer.axis.tiltSensitivity;
    break;
  case CONFIG_LED_TYPE:
    dataToReadWrite = &config_pointer.main.fretLEDMode;
    break;
  case CONFIG_MAP_JOY_DPAD:
    dataToReadWrite = (uint8_t *)&config_pointer.main.mapLeftJoystickToDPad;
    break;
  case CONFIG_MAP_START_SEL_HOME:
    dataToReadWrite = (uint8_t *)&config_pointer.main.mapStartSelectToHome;
    break;
  case CONFIG_MAP_ACCEL_RIGHT:
    dataToReadWrite = (uint8_t *)&config_pointer.main.mapNunchukAccelToRightJoy;
    break;
  case CONFIG_PIN_UP:
    dataToReadWrite = &config_pointer.pins.up;
    break;
  case CONFIG_PIN_DOWN:
    dataToReadWrite = &config_pointer.pins.down;
    break;
  case CONFIG_PIN_LEFT:
    dataToReadWrite = &config_pointer.pins.left;
    break;
  case CONFIG_PIN_RIGHT:
    dataToReadWrite = &config_pointer.pins.right;
    break;
  case CONFIG_PIN_START:
    dataToReadWrite = &config_pointer.pins.start;
    break;
  case CONFIG_PIN_SELECT:
    dataToReadWrite = &config_pointer.pins.back;
    break;
  case CONFIG_PIN_LEFT_STICK:
    dataToReadWrite = &config_pointer.pins.left_stick;
    break;
  case CONFIG_PIN_RIGHT_STICK:
    dataToReadWrite = &config_pointer.pins.right_stick;
    break;
  case CONFIG_PIN_LB:
    dataToReadWrite = &config_pointer.pins.LB;
    break;
  case CONFIG_PIN_RB:
    dataToReadWrite = &config_pointer.pins.RB;
    break;
  case CONFIG_PIN_HOME:
    dataToReadWrite = &config_pointer.pins.home;
    break;
  case CONFIG_PIN_CAPTURE:
    dataToReadWrite = &config_pointer.pins.capture;
    break;
  case CONFIG_PIN_A:
    dataToReadWrite = &config_pointer.pins.a;
    break;
  case CONFIG_PIN_B:
    dataToReadWrite = &config_pointer.pins.b;
    break;
  case CONFIG_PIN_X:
    dataToReadWrite = &config_pointer.pins.x;
    break;
  case CONFIG_PIN_Y:
    dataToReadWrite = &config_pointer.pins.y;
    break;
  case CONFIG_PIN_LT:
    dataToReadWrite = &config_pointer.pins.lt.pin;
    break;
  case CONFIG_PIN_RT:
    dataToReadWrite = &config_pointer.pins.rt.pin;
    break;
  case CONFIG_PIN_L_X:
    dataToReadWrite = &config_pointer.pins.l_x.pin;
    break;
  case CONFIG_PIN_L_Y:
    dataToReadWrite = &config_pointer.pins.l_y.pin;
    break;
  case CONFIG_PIN_R_X:
    dataToReadWrite = &config_pointer.pins.r_x.pin;
    break;
  case CONFIG_PIN_R_Y:
    dataToReadWrite = &config_pointer.pins.r_y.pin;
    break;
  case CONFIG_KEY_UP:
    dataToReadWrite = &config_pointer.keys.up;
    break;
  case CONFIG_KEY_DOWN:
    dataToReadWrite = &config_pointer.keys.down;
    break;
  case CONFIG_KEY_LEFT:
    dataToReadWrite = &config_pointer.keys.left;
    break;
  case CONFIG_KEY_RIGHT:
    dataToReadWrite = &config_pointer.keys.right;
    break;
  case CONFIG_KEY_START:
    dataToReadWrite = &config_pointer.keys.start;
    break;
  case CONFIG_KEY_SELECT:
    dataToReadWrite = &config_pointer.keys.back;
    break;
  case CONFIG_KEY_LEFT_STICK:
    dataToReadWrite = &config_pointer.keys.left_stick;
    break;
  case CONFIG_KEY_RIGHT_STICK:
    dataToReadWrite = &config_pointer.keys.right_stick;
    break;
  case CONFIG_KEY_LB:
    dataToReadWrite = &config_pointer.keys.LB;
    break;
  case CONFIG_KEY_RB:
    dataToReadWrite = &config_pointer.keys.RB;
    break;
  case CONFIG_KEY_HOME:
    dataToReadWrite = &config_pointer.keys.home;
    break;
  case CONFIG_KEY_CAPTURE:
    dataToReadWrite = &config_pointer.keys.capture;
    break;
  case CONFIG_KEY_A:
    dataToReadWrite = &config_pointer.keys.a;
    break;
  case CONFIG_KEY_B:
    dataToReadWrite = &config_pointer.keys.b;
    break;
  case CONFIG_KEY_X:
    dataToReadWrite = &config_pointer.keys.x;
    break;
  case CONFIG_KEY_Y:
    dataToReadWrite = &config_pointer.keys.y;
    break;
  case CONFIG_KEY_LT:
    dataToReadWrite = &config_pointer.keys.lt;
    break;
  case CONFIG_KEY_RT:
    dataToReadWrite = &config_pointer.keys.rt;
    break;
  case CONFIG_KEY_L_X:
    currentCommandSize = 2;
    dataToReadWrite = &config_pointer.keys.l_x.neg;
    break;
  case CONFIG_KEY_L_Y:
    currentCommandSize = 2;
    dataToReadWrite = &config_pointer.keys.l_y.neg;
    break;
  case CONFIG_KEY_R_X:
    currentCommandSize = 2;
    dataToReadWrite = &config_pointer.keys.r_x.neg;
    break;
  case CONFIG_KEY_R_Y:
    currentCommandSize = 2;
    dataToReadWrite = &config_pointer.keys.r_y.neg;
    break;
  case CONFIG_AXIS_INVERT_LT:
    dataToReadWrite = (uint8_t *)&config_pointer.pins.lt.inverted;
    break;
  case CONFIG_AXIS_INVERT_RT:
    dataToReadWrite = (uint8_t *)&config_pointer.pins.rt.inverted;
    break;
  case CONFIG_AXIS_INVERT_L_X:
    dataToReadWrite = (uint8_t *)&config_pointer.pins.l_x.inverted;
    break;
  case CONFIG_AXIS_INVERT_L_Y:
    dataToReadWrite = (uint8_t *)&config_pointer.pins.l_y.inverted;
    break;
  case CONFIG_AXIS_INVERT_R_X:
    dataToReadWrite = (uint8_t *)&config_pointer.pins.r_x.inverted;
    break;
  case CONFIG_AXIS_INVERT_R_Y:
    dataToReadWrite = (uint8_t *)&config_pointer.pins.r_y.inverted;
    break;
  case CONFIG_THRESHOLD_JOY:
    dataToReadWrite = (uint8_t *)&config_pointer.axis.joyThreshold;
    break;
  case CONFIG_THRESHOLD_TRIGGER:
    dataToReadWrite = (uint8_t *)&config_pointer.axis.triggerThreshold;
    break;
  case CONFIG_THRESHOLD_DRUM:
    dataToReadWrite = (uint8_t *)&config_pointer.drumThreshold;
    break;
  case CONFIG_LED_COLOURS:
    dataToReadWrite = (uint8_t *)&config_pointer.leds.colours;
    hasExtraData = true;
    currentCommandSize = 4;
    break;
  case CONFIG_LED_PINS:
    dataToReadWrite = (uint8_t *)&config_pointer.leds.pins;
    hasExtraData = true;
    break;
  case CONFIG_MIDI_CHANNEL:
    dataToReadWrite = (uint8_t *)&config_pointer.midi.channel;
    hasExtraData = true;
    break;
  case CONFIG_MIDI_NOTE:
    dataToReadWrite = (uint8_t *)&config_pointer.midi.note;
    hasExtraData = true;
    break;
  case CONFIG_MIDI_TYPE:
    dataToReadWrite = (uint8_t *)&config_pointer.midi.midiType;
    hasExtraData = true;
    break;
  }
}
bool processCommand(uint8_t cmd) {
  currentCommandSize = 0;
  hasExtraData = false;
  dataInRam = false;
  reading = true;
  started = false;
  switch (cmd) {
  case COMMAND_REBOOT:
    reboot();
    break;
  case COMMAND_JUMP_BOOTLOADER:
    bootloader();
    break;
  case COMMAND_SET_LED_COLOUR: {
    currentCommand = COMMAND_SET_LED_COLOUR;
    int i = 0;
    while (config.leds.pins[i]) { i++; }
    currentCommandSize = i * 4;
    dataInRam = true;
    dataToReadWrite = (uint8_t *)&controller.leds;
    reading = false;
    return true;
  }
  case COMMAND_FIND_DIGITAL:
    findDigitalPin();
    currentCommand = 0;
    reading = false;
    break;
  case COMMAND_FIND_ANALOG:
    findAnalogPin();
    currentCommand = 0;
    reading = false;
    break;
  case COMMAND_FIND_STOP:
    stopSearching();
    currentCommand = 0;
    reading = false;
    break;
  case COMMAND_WRITE_CONFIG_VALUE:
    reading = false;
  case COMMAND_READ_INFO:
  case COMMAND_READ_CONFIG_VALUE:
    currentCommand = cmd;
    return true;
  default:
    currentCommand = 0;
    break;
  }
  return false;
}
bool processCommandData(uint8_t data) {
  switch (currentCommand) {
  case COMMAND_WRITE_CONFIG_VALUE:
    get_config_buf(data);
    return true;
  case COMMAND_READ_INFO:
    get_info_buf(data);
    return false;
  case COMMAND_READ_CONFIG_VALUE:
    get_config_buf(data);
    return hasExtraData;
  }
  return false;
}
bool processMultiByteCommandData(uint8_t data) {
  if (hasExtraData) {
    dataToReadWrite += data * currentCommandSize;
    hasExtraData = false;
    return !reading;
  }
  return false;
}
void processSerialData(uint8_t data) {
  if (currentCommand == 0 && processCommand(data)) { return; }
  if (!started && currentCommandSize == 0 && processCommandData(data)) { return; }
  if (!started && hasExtraData && processMultiByteCommandData(data)) { return; }
  started = true;
  if (reading) {
    while (currentCommandSize--) {
      if (dataInRam) {
        writeToSerial(*(dataToReadWrite++));
      } else if (currentCommand == COMMAND_READ_INFO) {
        writeToSerial(pgm_read_byte(constDataToRead++));
      } else {
        writeToSerial(eeprom_read_byte(dataToReadWrite++));
      }
    }
  } else if (currentCommandSize--) {
    if (dataInRam) {
      *(dataToReadWrite++) = data;
    } else {
      eeprom_update_byte(dataToReadWrite++, data);
    }
    if (currentCommandSize) return;
  }
  writeToSerial('\r');
  writeToSerial('\n');
  currentCommand = 0;
}
// THIS IS SUPER BUGGY