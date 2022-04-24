#include "serial_handler.h"
#include "avr-nrf24l01/src/nrf24l01-mnemonics.h"
#include "avr-nrf24l01/src/nrf24l01.h"
#include "controller/controller.h"
#include "leds/leds.h"
#include "rf/rf.h"
#include "serial_commands.h"
#include "timer/timer.h"
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
  case COMMAND_FIND_ANALOG:
    if (!isRF) { findAnalogPin(); }
    break;
  case COMMAND_FIND_DIGITAL:
    if (!isRF) { findDigitalPin(); }
    break;
  case COMMAND_WRITE_CONFIG:
    return false;
  case COMMAND_RESET:
    resetConfig();
    return false;
  }
  return true;
}
void processHIDWriteFeatureReport(uint8_t cmd, uint8_t data_len,
                                  const uint8_t *data) {
  switch (cmd) {
  case COMMAND_WRITE_CONFIG: {
    uint16_t offset = (*data) * PACKET_SIZE;
    data++;
    data_len--;
    writeConfigBlock(offset, data, data_len);
    return;
  }
  case COMMAND_SET_LEDS: {
    uint16_t offset = (*data) * PACKET_SIZE;
    data++;
    data_len--;
    uint8_t *dest = ((uint8_t *)leds) + offset;
    while (data_len--) { *(dest++) = *(data++); }
    return;
  }
  case COMMAND_SET_SP: {
    setSP(data[1]);
  }
  }
  handleCommand(cmd);
}
const uint8_t PROGMEM err[] = "ERROR";
uint8_t dbuf[64];
void processHIDReadFeatureReport(uint8_t cmd, uint8_t report,
                                 const void *request) {
  if (isRF && cmd < COMMAND_READ_CONFIG && cmd != COMMAND_GET_CPU_INFO) {
    uint8_t dbuf2[2];
    dbuf2[0] = cmd;
    dbuf2[1] = true;
    uint8_t len;
    nrf24_flush_tx();
    nrf24_flush_rx();
    unsigned long ms = millis();
    while (true) {
      if (!nrf24_txFifoFull()) { nrf24_writeAckPayload(dbuf2, 2); }
      rf_interrupt = true;
      len = tickRFInput(dbuf, 0);
      if (len && len != sizeof(XInput_Data_t)) break;
      nrf24_configRegister(STATUS, (1 << TX_DS) | (1 << MAX_RT));
      if (millis() - ms > 500) {
        dbuf[0] = REPORT_ID_CONTROL;
        len = sizeof(err) + 1;
        memcpy_P(dbuf + 1, err, sizeof(err));
        break;
      }
    }
    writeToUSB(dbuf, len, report, request);
    return;
  }
  uint8_t size;
  dbuf[0] = REPORT_ID_CONTROL;
  if (cmd >= COMMAND_READ_CONFIG) {
    size = 50;
    uint16_t index = size * (cmd - COMMAND_READ_CONFIG);
    int16_t size2 = sizeof(Configuration_t) - index;
    if (size2 < size) { size = size2; }
    readConfigBlock(index, dbuf + 1, size);
    size = size + 1;
  } else if (cmd == COMMAND_GET_CPU_INFO || cmd == COMMAND_GET_RF_CPU_INFO) {
    size = sizeof(cpu_info_t) + 1;
    cpu_info_t *info = (cpu_info_t *)(dbuf + 1);
    strcpy_P(info->board, PSTR(ARDWIINO_BOARD));
    info->cpu_freq = F_CPU;
    info->rfID = generate_crc32();
#ifdef MULTI_ADAPTOR
    info->multi = true;
#else
    info->multi = false;
#endif
  } else if (cmd == COMMAND_GET_VALUES) {
    memcpy(dbuf + 1, &analogueData, sizeof(analogueData));
    size = sizeof(analogueData) + 1;
  } else if (cmd == COMMAND_GET_EXTENSION) {
    size = 3;
    if (inputType == WII) {
      dbuf[1] = wiiExtensionID & 0xff;
      dbuf[2] = wiiExtensionID >> 8;
    } else if (inputType == PS2) {
      dbuf[1] = ps2CtrlType & 0xff;
      dbuf[2] = 0;
    }
  } else if (cmd == COMMAND_GET_FOUND) {
    size = 2;
    dbuf[1] = detectedPin;
    stopSearching();
  } else {
    size = sizeof(id) + 1;
    memcpy_P(dbuf + 1, id, sizeof(id));
    if (fullDeviceType <= PS3_ROCK_BAND_DRUMS) {
      dbuf[4] = 0x00;
    } else if (fullDeviceType <= PS3_GUITAR_HERO_DRUMS) {
      dbuf[4] = 0x06;
    }
  }
  writeToUSB(dbuf, size, report, request);
}