#include "serial_handler.h"
static char *FW = ARDWIINO_BOARD;
int16_t process_serial(USB_ClassInfo_CDC_Device_t *VirtualSerial_CDC_Interface) {
  int16_t b = CDC_Device_ReceiveByte(VirtualSerial_CDC_Interface);
  uint8_t *from;
  uint8_t *to;
  uint16_t size = 0;
  bool w = false;
  switch (b) {
  case MAIN_CMD_W:
    w = true;
  case MAIN_CMD_R:
    from = (uint8_t*)&config.main;
    to = (uint8_t*)&config_pointer.main;
    size = sizeof(main_config_t);
    break;
  case PIN_CMD_W:
    w = true;
  case PIN_CMD_R:
    from = (uint8_t*)&config.pins;
    to = (uint8_t*)&config_pointer.pins;
    size = sizeof(pins_t);
    break;
  case AXIS_CMD_W:
    w = true;
  case AXIS_CMD_R:
    from = (uint8_t*)&config.axis;
    to = (uint8_t*)&config_pointer.axis;
    size = sizeof(axis_config_t);
    break;
  case KEY_CMD_W:
    w = true;
  case KEY_CMD_R:
    from = (uint8_t*)&config.keys;
    to = (uint8_t*)&config_pointer.keys;
    size = sizeof(keys_t);
    break;
  case CONTROLLER_CMD_R:
    from = (uint8_t*)&controller;
    size = sizeof(controller_t);
    break;
  case FW_CMD_R:
    from = (uint8_t*)FW;
    size = strlen(FW);
    break;
  case REBOOT_CMD:
    reboot();
    break;
  }
  if (size > 0) {
    if (w) {
      CDC_Device_SendString(VirtualSerial_CDC_Interface, "READY");
      while (size > 0) {
        eeprom_update_byte(to++, CDC_Device_ReceiveByte(VirtualSerial_CDC_Interface));
        size--;
      }
      CDC_Device_SendString(VirtualSerial_CDC_Interface, "OK");
    } else {
      CDC_Device_SendData(VirtualSerial_CDC_Interface, from, size);
    }
  }
  CDC_Device_USBTask(VirtualSerial_CDC_Interface);
  USB_USBTask();
  return b;
}