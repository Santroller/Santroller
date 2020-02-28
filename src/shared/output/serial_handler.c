#include "serial_handler.h"
#include "../input/input_wii_ext.h"
#include "usb/API.h"
#include <stdlib.h>

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
    case INFO_USB_MCU:
      buf = (uint8_t *)usb_mcu;
      break;
    case INFO_USB_CPU_FREQ:
      buf = (uint8_t *)usb_freq;
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
      uint8_t str[50];
      get_wii_device_name((char*)str);
      buf = str;
    } break;
    }
  }
  if (buf != NULL) {
    while (*(buf)) { write_usb(*(buf++)); }
    write_usb('\n');
  }
}