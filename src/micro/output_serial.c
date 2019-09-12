#include "../shared/output/output_serial.h"
#include "../shared/config/eeprom.h"
#include "../shared/output/bootloader/bootloader.h"
#include "../shared/output/output_handler.h"
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

static char* FW = "Micro";
void serial_tick() {

  int16_t b = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);
  if (b == 'c') {
    CDC_Device_SendData(&VirtualSerial_CDC_Interface, &config,
                        sizeof(config_t));
  } else if (b == 'r') {
    CDC_Device_SendData(&VirtualSerial_CDC_Interface, &controller,
                        sizeof(controller_t));
  } else if (b == 'f') {
    CDC_Device_SendString(&VirtualSerial_CDC_Interface, FW);
  } else if (b == 'w') {
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

void EVENT_CDC_Device_ControLineStateChanged(USB_ClassInfo_CDC_Device_t *const CDCInterfaceInfo)
{
	/* You can get changes to the virtual CDC lines in this callback; a common
	   use-case is to use the Data Terminal Ready (DTR) flag to enable and
	   disable CDC communications in your application when set to avoid the
	   application blocking while waiting for a host to become ready and read
	   in the pending data from the USB endpoints.
	*/
	bool HostReady = (CDCInterfaceInfo->State.ControlLineStates.HostToDevice & CDC_CONTROL_LINE_OUT_DTR) == 0;
  if (HostReady && CDCInterfaceInfo->State.LineEncoding.BaudRateBPS == 1200) {
    bootloader();
  }
}