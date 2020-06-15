#include "serial_handler.h"
#include "input/inputs/direct.h"
#include "input/inputs/ps2_cnt.h"
#include "input/inputs/wii_ext.h"
#include "serial_commands.h"
#include "util/util.h"
#include <stdlib.h>
static uint8_t id[] = {0x21, 0x26, 0x01, 0x07, 0x00, 0x00, 0x00, 0x00};
int currentCommand = 0;
int currentCommandSize = 0;
static const uint8_t *constDataToRead = NULL;
static uint8_t *dataToReadWrite = NULL;
bool dataInRam = false;
void getData(uint8_t report) {
  constDataToRead = NULL;
  currentCommandSize = 1;
  switch (report) {
  case COMMAND_GET_SIGNATURE:
    constDataToRead = (const uint8_t *)PSTR(SIGNATURE);
    break;
  case COMMAND_GET_CPU_FREQ:
    constDataToRead = (const uint8_t *)PSTR(STR(F_CPU));
    break;
  case COMMAND_GET_BOARD:
    constDataToRead = (const uint8_t *)PSTR(ARDWIINO_BOARD);
    break;
  case COMMAND_GET_EXTENSION:
    dataInRam = true;
    if (config.main.inputType == WII) {
      dataToReadWrite = (uint8_t *)&wiiExtensionID;
      currentCommandSize = 2;
    } else if (config.main.inputType == PS2) {
      dataToReadWrite = (uint8_t *)&ps2CtrlType;
    } else {
      currentCommandSize = 0;
    }
    break;
  case DATA_INPUT_TYPE:
    dataToReadWrite = &config_pointer.main.inputType;
    break;
  case DATA_SUB_TYPE:
    dataToReadWrite = &config_pointer.main.subType;
    break;
  case DATA_TILT_TYPE:
    dataToReadWrite = &config_pointer.main.tiltType;
    break;
  case DATA_MPU_6050_ORIENTATION:
    dataToReadWrite = &config_pointer.axis.mpu6050Orientation;
    break;
  case DATA_TILT_SENSITIVITY:
    dataToReadWrite = (uint8_t *)&config_pointer.axis.tiltSensitivity;
    break;
  case DATA_LED_TYPE:
    dataToReadWrite = &config_pointer.main.fretLEDMode;
    break;
  case DATA_MAP_JOY_DPAD:
    dataToReadWrite = (uint8_t *)&config_pointer.main.mapLeftJoystickToDPad;
    break;
  case DATA_MAP_START_SEL_HOME:
    dataToReadWrite = (uint8_t *)&config_pointer.main.mapStartSelectToHome;
    break;
  case DATA_MAP_ACCEL_RIGHT:
    dataToReadWrite = (uint8_t *)&config_pointer.main.mapNunchukAccelToRightJoy;
    break;
  case DATA_PINS:
    dataToReadWrite = (uint8_t *)&config_pointer.pins;
    currentCommandSize = sizeof(config.pins);
    break;
  case DATA_KEYS:
    dataToReadWrite = (uint8_t *)&config_pointer.keys;
    currentCommandSize = sizeof(config.keys);
    break;
  case DATA_THRESHOLD_JOY:
    dataToReadWrite = (uint8_t *)&config_pointer.axis.joyThreshold;
    break;
  case DATA_THRESHOLD_TRIGGER:
    dataToReadWrite = (uint8_t *)&config_pointer.axis.triggerThreshold;
    break;
  case DATA_THRESHOLD_DRUM:
    dataToReadWrite = (uint8_t *)&config_pointer.drumThreshold;
    break;

  case DATA_LED_COLOURS:
    dataToReadWrite = (uint8_t *)&config_pointer.leds.colours;
    currentCommandSize = sizeof(config.leds.colours);
    break;
  case DATA_LED_PINS:
    dataToReadWrite = (uint8_t *)&config_pointer.leds.pins;
    currentCommandSize = sizeof(config.leds.pins);
    break;
  case DATA_MIDI_CHANNEL:
    dataToReadWrite = (uint8_t *)&config_pointer.midi.channel;
    currentCommandSize = sizeof(config.midi.channel);
    break;
  case DATA_MIDI_NOTE:
    dataToReadWrite = (uint8_t *)&config_pointer.midi.note;
    currentCommandSize = sizeof(config.midi.note);
    break;
  case DATA_MIDI_TYPE:
    dataToReadWrite = (uint8_t *)&config_pointer.midi.midiType;
    currentCommandSize = sizeof(config.midi.midiType);
    break;
  }

  if (constDataToRead) {
    currentCommandSize = strlen_P((char *)constDataToRead);
  }
}
extern uint8_t dbuf[sizeof(USB_Descriptor_Configuration_t)];
void processHIDWriteFeatureReport(uint8_t report, uint8_t data_len,
                                  uint8_t *data) {
  uint8_t cmd = *data;
  data++;
  switch (report) {
  case REPORT_GET_PS3_ID:
    if (config.main.subType <= PS3_ROCK_BAND_DRUMS) {
      id[3] = 0x00;
    } else if (config.main.subType <= PS3_GUITAR_HERO_DRUMS) {
      id[3] = 0x00;
    }
    dataInRam = true;
    dataToReadWrite = id;
    currentCommandSize = sizeof(id);
  case REPORT_SET_CONFIG:
    getData(cmd);
    break;
  case REPORT_GET_CONFIG:
    getData(cmd);
    if (dataInRam) {
      memcpy(dbuf, dataToReadWrite, currentCommandSize);
    } else if (constDataToRead) {
      memcpy_P(dbuf, constDataToRead, currentCommandSize);
    } else {
      eeprom_read_block(dbuf, dataToReadWrite, currentCommandSize);
    }
    return;
  default:
    return;
  }
  switch (cmd) {
  case COMMAND_REBOOT:
    reboot();
    return;
  case COMMAND_JUMP_BOOTLOADER:
    bootloader();
    return;
  case COMMAND_FIND_DIGITAL:
    findDigitalPin();
    return;
  case COMMAND_FIND_ANALOG:
    findAnalogPin();
    return;
  case COMMAND_FIND_STOP:
    stopSearching();
    return;
  }
  if (dataInRam) {
    memcpy(dataToReadWrite, data, currentCommandSize);
  } else {
    eeprom_write_block(data, dataToReadWrite, currentCommandSize);
  }
}
void processHIDReadFeatureReport(uint8_t report) {
  Endpoint_Write_Control_Stream_LE(dbuf, currentCommandSize);
}