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
bool handleCommand(uint8_t cmd) {
  switch (cmd) {
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
void processHIDWriteFeatureReport(uint8_t data_len, uint8_t *data) {
  uint8_t cmd = *data;
  data++;
  data_len--;
  switch (cmd) {
  case COMMAND_WRITE_CONFIG: {
    uint8_t offset = *data;
    data++;
    data_len--;
    eeprom_update_block(data, ((uint8_t *)&config_pointer) + offset, data_len);
    return;
  }
  case COMMAND_SET_LEDS: {
    uint8_t *dest = (uint8_t *)controller.leds;
    while (data_len--) { *(dest++) = *(data++); }
    return;
  }
  }
  handleCommand(cmd);
}
void processHIDReadFeatureReport(uint8_t cmd) {
  uint16_t size;
  dbuf[0] = REPORT_ID_CONTROL;
  if (cmd == COMMAND_GET_CPU_INFO) {
     size = sizeof(cpu_info_t)+1;
     cpu_info_t* info = (cpu_info_t*)(dbuf+1);
     strcpy_P(info->board, PSTR(ARDWIINO_BOARD));
     strcpy_P(info->mcu, PSTR(MCU));
     info->cpu_freq = F_CPU;
     #ifdef MULTI_ADAPTOR
      info->multi = true;
     #else
      info->multi = false;
     #endif
  } else {
    size = sizeof(id);
    memcpy_P(dbuf, id, sizeof(id));
    if (config.main.subType <= PS3_ROCK_BAND_DRUMS) {
      dbuf[3] = 0x00;
    } else if (config.main.subType <= PS3_GUITAR_HERO_DRUMS) {
      dbuf[3] = 0x06;
    }
  }
  // data->cpu_freq = F_CPU;
  // data->detectedPin = detectedPin;
  // if (config.main.inputType == WII) {
  //   data->extension = wiiExtensionID;
  // } else if (config.main.inputType == PS2) {
  //   data->extension = ps2CtrlType;
  // }
  // strcpy_P((char*)data->board, PSTR(ARDWIINO_BOARD));
  writeToUSB(dbuf, size);
}