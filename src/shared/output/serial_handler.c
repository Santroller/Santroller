#include "serial_handler.h"
int16_t process_serial(USB_ClassInfo_CDC_Device_t *VirtualSerial_CDC_Interface) {
  int16_t b = CDC_Device_ReceiveByte(VirtualSerial_CDC_Interface);
  
  return b;
}