#include "../shared/output/output_serial.h"
#include "../shared/output/output_handler.h"
#include "../shared/config/eeprom.h"
#include "../shared/output/bootloader/bootloader.h"
/** LUFA CDC Class driver interface configuration and state information. This
 * structure is passed to all CDC Class driver functions, so that multiple
 * instances of the same class within a device can be differentiated from one
 * another.
 */
USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface = {
    .Config =
        {
            .ControlInterfaceNumber = INTERFACE_ID_CDC_CCI,
            .DataINEndpoint =
                {
                    .Address = CDC_TX_EPADDR,
                    .Size = CDC_TXRX_EPSIZE,
                    .Banks = 1,
                },
            .DataOUTEndpoint =
                {
                    .Address = CDC_RX_EPADDR,
                    .Size = CDC_TXRX_EPSIZE,
                    .Banks = 1,
                },
            .NotificationEndpoint =
                {
                    .Address = CDC_NOTIFICATION_EPADDR,
                    .Size = CDC_NOTIFICATION_EPSIZE,
                    .Banks = 1,
                },
        },
};
void serial_configuration_changed() {
  CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);
}
void serial_control_request() {
  CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
}
void serial_init(controller_t *c) {}
void serial_tick() {

  int16_t b = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);
  if (b > 0) { CDC_Device_SendByte(&VirtualSerial_CDC_Interface, b); }
  if (b == 'b') { bootloader(); }
  if (b == 'r') {
    CDC_Device_SendData(&VirtualSerial_CDC_Interface, &config,
                        sizeof(config_t));
  }
  if (b == 'i') {
    CDC_Device_SendData(&VirtualSerial_CDC_Interface, &controller,
                        sizeof(controller_t));
  }
  if (b == 'w') {
    uint8_t *data = (uint8_t *)&config;
    size_t i = 0;
    while (i < sizeof(config_t)) {
      data[i] = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);
      if (data[i] != EOF) { i++; }
    }
    reboot();
  }
  CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
}