#include "serial_handler.h"
#include "input/inputs/direct.h"
#include "input/inputs/ps2_cnt.h"
#include "input/inputs/wii_ext.h"
#include "leds/leds.h"
#include "serial_commands.h"
#include "util/util.h"
#include <stdlib.h>
static const uint8_t PROGMEM id[] = {0x21, 0x26, 0x01, 0x07,
                                     0x00, 0x00, 0x00, 0x00};
bool getData(uint8_t report) {
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
    break;
  case COMMAND_FIND_DIGITAL:
    findDigitalPin();
    break;
  case COMMAND_WRITE_CONFIG:
    return false;
  }
  return true;
}
bool processHIDWriteFeatureReport(uint8_t data_len, uint8_t *data) {
  uint8_t report = *data;
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
  const char *board = PSTR(ARDWIINO_BOARD);
  const char *sig = PSTR(SIGNATURE);
  data_t *data = (data_t *)dbuf;
  memcpy_P(data->ps3id, id, sizeof(id));
  if (config.main.subType <= PS3_ROCK_BAND_DRUMS) {
    data->ps3id[3] = 0x00;
  } else if (config.main.subType <= PS3_GUITAR_HERO_DRUMS) {
    data->ps3id[3] = 0x06;
  }
  data->cpu_freq = F_CPU;
  data->detectedPin = detectedPin;
  if (config.main.inputType == WII) {
    data->extension = wiiExtensionID;
  } else if (config.main.inputType == PS2) {
    data->extension = ps2CtrlType;
  }
  memcpy(&data->conf, &config, sizeof(config));
  memcpy_P(data->board, board, sizeof(board));
  memcpy_P(data->signature, sig, sizeof(sig));
  writeToUSB(dbuf, sizeof(data_t));
}