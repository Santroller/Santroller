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
  dataInRam = false;
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
  case COMMAND_CONFIG:
    dataToReadWrite = (uint8_t *)&config_pointer;
    currentCommandSize = sizeof(Configuration_t);
    break;
  case COMMAND_GET_PS3_ID:
    if (config.main.subType <= PS3_ROCK_BAND_DRUMS) {
      id[3] = 0x00;
    } else if (config.main.subType <= PS3_GUITAR_HERO_DRUMS) {
      id[3] = 0x00;
    }
    dataInRam = true;
    dataToReadWrite = id;
    currentCommandSize = sizeof(id);
  }

  if (constDataToRead) {
    currentCommandSize = strlen_P((char *)constDataToRead);
  }
}
void processHIDWriteFeatureReport(uint8_t report, uint8_t data_len,
                                  uint8_t *data) {
  switch (report) {
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
  case COMMAND_FIND_CANCEL:
    stopSearching();
    return;
  }
  getData(report);
  if (dataInRam) {
    memcpy(dataToReadWrite, data, currentCommandSize);
  } else {
    eeprom_write_block(data, dataToReadWrite, currentCommandSize);
  }
}
void processHIDReadFeatureReport(uint8_t report) {
  switch (report) {
  case COMMAND_FIND_ANALOG:
  case COMMAND_FIND_DIGITAL:
    dataInRam = true;
    dataToReadWrite = &detectedPin;
  default:
    getData(report);
  }
  if (dataInRam) {
    memcpy(dbuf, dataToReadWrite, currentCommandSize);
  } else if (constDataToRead) {
    memcpy_P(dbuf, constDataToRead, currentCommandSize);
  } else {
    eeprom_read_block(dbuf, dataToReadWrite, currentCommandSize);
  }
  Endpoint_Write_Control_Stream_LE(dbuf, currentCommandSize);
}