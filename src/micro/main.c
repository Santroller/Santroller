#include "../shared/config/eeprom.h"
#include "../shared/input/input_handler.h"
#include "../shared/output/bootloader/bootloader.h"
#include "../shared/output/control_requests.h"
#include "../shared/output/reports.h"
#include "../shared/output/serial_handler.h"
#include "../shared/output/usb/Descriptors.h"
#include "../shared/util.h"
#include "../shared/Arduino.h"
#include "stdbool.h"
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
                    .Size = CDC_TX_EPSIZE,
                    .Banks = 1,
                },
            .DataOUTEndpoint =
                {
                    .Address = CDC_RX_EPADDR,
                    .Size = CDC_RX_EPSIZE,
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
uint8_t detected_pin;
bool found_pin = false;
output_report_size_t last_report;
USB_ClassInfo_HID_Device_t interface = {
  Config : {
    InterfaceNumber : INTERFACE_ID_HID,
    ReportINEndpoint : {
      Address : HID_EPADDR_IN,
      Size : HID_EPSIZE,
      Type : EP_TYPE_CONTROL,
      Banks : 1,
    },
    PrevReportINBuffer : &last_report,
    PrevReportINBufferSize : sizeof(output_report_size_t),
  },
};
USB_ClassInfo_MIDI_Device_t Keyboard_MIDI_Interface = {
    .Config =
        {
            .StreamingInterfaceNumber = INTERFACE_ID_AudioStream,
            .DataINEndpoint =
                {
                    .Address = HID_EPADDR_IN,
                    .Size = HID_EPSIZE,
                    .Banks = 1,
                },
            .DataOUTEndpoint =
                {
                    .Address = HID_EPADDR_OUT,
                    .Size = HID_EPSIZE,
                    .Banks = 1,
                },
        },
};
controller_t controller;
output_report_size_t report;
int main(void) {
  load_config();
  device_type = config.main.sub_type;
  input_init();
  report_init();
  USB_Init();
  sei();
  uint16_t size;
  uint16_t rec;
  while (true) {
    input_tick(&controller);
    create_report(&report, &size, &controller);
    if (memcmp(&report, &last_report, size) != 0) {
      memcpy(&last_report, &report, size);
      Endpoint_SelectEndpoint(HID_EPADDR_IN);
      if (Endpoint_IsReadWriteAllowed()) {
        Endpoint_Write_Stream_LE(&report, size, NULL);
        Endpoint_ClearIN();
      }
    }

    rec = CDC_Device_BytesReceived(&VirtualSerial_CDC_Interface);
    while (rec--) {
      process_serial(CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface) &
                     0xff);
    }
    if (found_pin) {
      found_pin = false;
      write_usb('d');
      write_usb(detected_pin);
      write_usb('\r');
      write_usb('\n');
    }
    CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
    MIDI_Device_USBTask(&Keyboard_MIDI_Interface);
  }
}

void write_usb(uint8_t data) {
  CDC_Device_SendByte(&VirtualSerial_CDC_Interface, data);
}
void EVENT_USB_Device_ConfigurationChanged(void) {
  if (device_type >= MIDI_GUITAR) {
    MIDI_Device_ConfigureEndpoints(&Keyboard_MIDI_Interface);
  } else {
    HID_Device_ConfigureEndpoints(&interface);
  }
  CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);
}
void EVENT_USB_Device_ControlRequest(void) {
  controller_control_request();
  CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
}
void EVENT_CDC_Device_ControLineStateChanged(
    USB_ClassInfo_CDC_Device_t *const CDCInterfaceInfo) {
  /* You can get changes to the virtual CDC lines in this callback; a common
     use-case is to use the Data Terminal Ready (DTR) flag to enable and
     disable CDC communications in your application when set to avoid the
     application blocking while waiting for a host to become ready and read
     in the pending data from the USB endpoints.
  */
  bool HostReady = (CDCInterfaceInfo->State.ControlLineStates.HostToDevice &
                    CDC_CONTROL_LINE_OUT_DTR) == 0;
  if (HostReady && CDCInterfaceInfo->State.LineEncoding.BaudRateBPS == 1200) {
    bootloader();
  }
}