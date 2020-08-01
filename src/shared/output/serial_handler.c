#include "serial_handler.h"
#include "input/inputs/direct.h"
#include "input/inputs/ps2_cnt.h"
#include "input/inputs/wii_ext.h"
#include "leds/leds.h"
#include "serial_commands.h"
#include "util/util.h"
#include <stdlib.h>
static uint8_t id[] = {0x21, 0x26, 0x01, 0x07, 0x00, 0x00, 0x00, 0x00};
int currentCommand = 0;
int currentCommandSize = 0;
static const uint8_t *constDataToRead = NULL;
static uint8_t *dataToReadWrite = NULL;
bool getData(uint8_t report) {
  constDataToRead = NULL;
  currentCommandSize = 0;
  switch (report) {
  case COMMAND_REBOOT:
    reboot();
    return false;
  case COMMAND_JUMP_BOOTLOADER:
    bootloader();
    return false;
  case COMMAND_FIND_CANCEL:
    stopSearching();
    return false;
  case COMMAND_FIND_ANALOG:
    findAnalogPin();
    dataToReadWrite = &detectedPin;
    currentCommandSize = 1;
    break;
  case COMMAND_FIND_DIGITAL:
    findDigitalPin();
    dataToReadWrite = &detectedPin;
    currentCommandSize = 1;
    break;
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
    if (config.main.inputType == WII) {
      dataToReadWrite = (uint8_t *)&wiiExtensionID;
      currentCommandSize = 2;
    } else if (config.main.inputType == PS2) {
      dataToReadWrite = (uint8_t *)&ps2CtrlType;
      currentCommandSize = 1;
    } else {
      currentCommandSize = 0;
    }
    break;
  case COMMAND_WRITE_CONFIG:
    return false;
  case COMMAND_READ_CONFIG:
    dataToReadWrite = (uint8_t *)&config;
    currentCommandSize = sizeof(Configuration_t);
    break;
  }

  if (constDataToRead) {
    currentCommandSize = strlen_P((char *)constDataToRead);
  }
  return true;
}
bool processHIDWriteFeatureReport(uint8_t data_len, uint8_t *data) {
  uint8_t report = *data;
  constDataToRead = NULL;
  currentCommandSize = 0;
  data++;
  data_len--;
  switch (report) {
  case COMMAND_WRITE_CONFIG: {
    uint8_t offset = *data;
    data++;
    data_len--;
    eeprom_update_block(data, ((uint8_t *)&config_pointer) + offset, data_len);
    return false;
  }
  case COMMAND_SET_LEDS: {
    uint8_t *dest = (uint8_t *)controller.leds;
    while (data_len--) { *(dest++) = *(data++); }
    return false;
  }
  }
  return getData(report);
}
void processHIDReadFeatureReport(void) {
  if (!currentCommandSize) {
    if (config.main.subType <= PS3_ROCK_BAND_DRUMS) {
      id[3] = 0x00;
    } else if (config.main.subType <= PS3_GUITAR_HERO_DRUMS) {
      id[3] = 0x06;
    }
    dataToReadWrite = id;
    currentCommandSize = sizeof(id);
  }
  if (constDataToRead) {
    memcpy_P(dbuf, constDataToRead, currentCommandSize);
  } else {
    memcpy(dbuf, dataToReadWrite, currentCommandSize);
  }
  writeToUSB(dbuf, currentCommandSize);
}