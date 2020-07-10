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
bool reading = false;
void getData(uint8_t report) {
  reading = true;
  dataInRam = false;
  constDataToRead = NULL;
  switch (report) {
  case COMMAND_FIND_ANALOG:
  case COMMAND_FIND_DIGITAL:
    dataInRam = true;
    dataToReadWrite = &detectedPin;
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
  case COMMAND_READ_CONFIG:
    dataToReadWrite = (uint8_t *)&config_pointer;
    currentCommandSize = sizeof(Configuration_t);
    break;
  }

  if (constDataToRead) {
    currentCommandSize = strlen_P((char *)constDataToRead);
  }
}
void processHIDWriteFeatureReport(uint8_t data_len, uint8_t *data) {
  uint8_t report = *data;
  data++;
  data_len--;
  switch (report) {
  case COMMAND_REBOOT:
    reboot();
    return;
  case COMMAND_JUMP_BOOTLOADER:
    bootloader();
    return;
  case COMMAND_FIND_DIGITAL:
    findDigitalPin();
    break;
  case COMMAND_FIND_ANALOG:
    findAnalogPin();
    break;
  case COMMAND_FIND_CANCEL:
    stopSearching();
    return;
  case COMMAND_WRITE_CONFIG: {
    uint8_t offset = *data;
    data++;
    data_len--;
    eeprom_write_block(data, ((uint8_t *)&config_pointer) + offset, data_len);
    return;
  }
  }
  getData(report);
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