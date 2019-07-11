#include "../../shared/output/output_serial.h"
#include "../../shared/config/eeprom.h"
#include "../../shared/output/bootloader/bootloader.h"
#include <LUFA/Drivers/Board/Board.h>
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
void serial_init() {}
void serial_tick() {

  int16_t b = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);
  if (b > 0) { CDC_Device_SendByte(&VirtualSerial_CDC_Interface, b); }
  if (b == 'b') { bootloader(); }
  if (b == 'r') {
    CDC_Device_SendData(&VirtualSerial_CDC_Interface, &config,
                        sizeof(config_t));
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

void serial_receive(uint8_t ReceivedByte) {
  CDC_Device_SendByte(&VirtualSerial_CDC_Interface, ReceivedByte);
}
/** Event handler for the CDC Class driver Host-to-Device Line Encoding Changed
 * event.
 *
 *  \param[in] CDCInterfaceInfo  Pointer to the CDC class interface
 * configuration structure being referenced
 */
void EVENT_CDC_Device_ControLineStateChanged(
    USB_ClassInfo_CDC_Device_t *const CDCInterfaceInfo) {
  bool CurrentDTRState =
      (CDCInterfaceInfo->State.ControlLineStates.HostToDevice &
       CDC_CONTROL_LINE_OUT_DTR);
  // At this point, we know if we are in programming mode or not
  if (CurrentDTRState)
    AVR_RESET_LINE_PORT &= ~AVR_RESET_LINE_MASK;
  else
    AVR_RESET_LINE_PORT |= AVR_RESET_LINE_MASK;
}
