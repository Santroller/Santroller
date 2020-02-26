#include "serial_handler.h"
#include "usb/API.h"
#include <stdlib.h>

const char *mcu = MCU;
const char *board = ARDWIINO_BOARD;
const char *version = VERSION;
const char *signature = SIGNATURE;
const char *freq = STR(F_CPU);
extern uint16_t id;
void process_serial(void) {
  const uint8_t *buf = NULL;
  switch (read_usb()) {
  case COMMAND_READ_INFO:
    switch (read_usb()) {
    case INFO_MAIN_MCU:
      buf = (uint8_t *)mcu;
      break;
    case INFO_CPU_FREQ:
      buf = (uint8_t *)freq;
      break;
    case INFO_BOARD:
      buf = (uint8_t *)board;
      break;
    case INFO_VERSION:
      buf = (uint8_t *)version;
      break;
    case INFO_SIGNATURE:
      buf = (uint8_t *)signature;
      break;
    case INFO_WII_EXT: {
      char str[4];
      itoa(id, str, 16);
      buf = (uint8_t *)str;
    } break;
    }
  }
  if (buf != NULL) {
    while (*(buf)) {
      write_usb(*(buf++));
    }
    write_usb('\n');
  }
}