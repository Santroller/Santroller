#include "../shared/config/eeprom.h"
#include "../shared/input/input_handler.h"
#include "../shared/output/bootloader/bootloader.h"
#include "../shared/output/control_requests.h"
#include "../shared/output/reports.h"
#include "../shared/output/serial_handler.h"
#include "../shared/output/usb/Descriptors.h"
#include "../shared/util.h"
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
controller_t controller;
int main(void) {
  load_config();
  init_serial();
  device_type = config.main.sub_type;
  polling_rate = config.main.poll_rate;
  input_init();
  report_init();
  USB_Init();
  sei();
  while (true) {
    input_tick(&controller);
    process_serial();
    HID_Device_USBTask(&interface);
    CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
  }
}
uint8_t read_usb(void) {
  return CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface) & 0xff;
}

bool can_read_usb(void) {
  return CDC_Device_BytesReceived(&VirtualSerial_CDC_Interface);
}

void write_usb(uint8_t data) {
  CDC_Device_SendByte(&VirtualSerial_CDC_Interface, data);
}
void EVENT_USB_Device_ConfigurationChanged(void) {
  HID_Device_ConfigureEndpoints(&interface);
  CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);
}
void EVENT_USB_Device_ControlRequest(void) {
  HID_Device_ProcessControlRequest(&interface);
  CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
  controller_control_request();
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
bool CALLBACK_HID_Device_CreateHIDReport(
    USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo, uint8_t *const ReportID,
    const uint8_t ReportType, void *ReportData, uint16_t *const ReportSize) {
  create_report(ReportData, ReportSize, controller);
  return true;
}

void CALLBACK_HID_Device_ProcessHIDReport(
    USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo, const uint8_t ReportID,
    const uint8_t ReportType, const void *ReportData,
    const uint16_t ReportSize) {}