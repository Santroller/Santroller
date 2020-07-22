#include "serial_handler.h"
#include "input/inputs/direct.h"
#include "input/inputs/ps2_cnt.h"
#include "input/inputs/wii_ext.h"
#include "serial_commands.h"
#include "util/util.h"
#include "leds/leds.h"
#include <stdlib.h>
static uint8_t id[] = {0x21, 0x26, 0x01, 0x07, 0x00, 0x00, 0x00, 0x00};
int currentCommand = 0;
int currentCommandSize = 0;
static const uint8_t *constDataToRead = NULL;
static uint8_t *dataToReadWrite = NULL;
bool dataInRam = false;
void getData(uint8_t report) {
  switch (report) {
  case COMMAND_FIND_ANALOG:
    findAnalogPin();
    dataInRam = true;
    dataToReadWrite = &detectedPin;
    currentCommandSize = 1;
    break;
  case COMMAND_FIND_DIGITAL:
    findDigitalPin();
    dataInRam = true;
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
    dataInRam = true;
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
  case COMMAND_READ_CONFIG:
    dataToReadWrite = (uint8_t *)&config_pointer;
    currentCommandSize = sizeof(Configuration_t);
    break;
  }

  if (constDataToRead) {
    currentCommandSize = strlen_P((char *)constDataToRead);
  }
}
bool processHIDWriteFeatureReport(uint8_t data_len, uint8_t *data) {
  uint8_t report = *data;
  dataInRam = false;
  constDataToRead = NULL;
  currentCommandSize = 0;
  data++;
  data_len--;
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
  case COMMAND_WRITE_CONFIG: {
    uint8_t offset = *data;
    data++;
    data_len--;
    eeprom_update_block(data, ((uint8_t *)&config_pointer) + offset, data_len);
    return false;
  }
  case COMMAND_SET_LEDS:
    memcpy(controller.leds, data, sizeof(controller.leds));
    tickLEDs(&controller);
    return false;
  }
  getData(report);
  return true;
}
void processHIDReadFeatureReport(void) {
  if (!currentCommandSize) {
    if (config.main.subType <= PS3_ROCK_BAND_DRUMS) {
      id[3] = 0x00;
    } else if (config.main.subType <= PS3_GUITAR_HERO_DRUMS) {
      id[3] = 0x06;
    }
    dataInRam = true;
    dataToReadWrite = id;
    currentCommandSize = sizeof(id);
  }
  if (dataInRam) {
    memcpy(dbuf, dataToReadWrite, currentCommandSize);
  } else if (constDataToRead) {
    memcpy_P(dbuf, constDataToRead, currentCommandSize);
  } else {
    eeprom_read_block(dbuf, dataToReadWrite, currentCommandSize);
  }
  writeToUSB(dbuf, currentCommandSize);
}