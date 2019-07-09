#include "output_handler.h"
#include "avr/wdt.h"
#include "bootloader/bootloader.h"
#include "output_hid.h"
#include "output_xinput.h"
#include "usb/Descriptors.h"

// static FILE USBSerialStream;
/** LUFA CDC Class driver interface configuration and state information. This
 * structure is passed to all CDC Class driver functions, so that multiple
 * instances of the same class within a device can be differentiated from one
 * another.
 */
USB_ClassInfo_CDC_Device_t SerialInterface = {
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
event_pointers events;
void output_init(void) {
  if (config.sub_type >= KEYBOARD_SUBTYPE) {
    hid_init(&events);
  } else {
    xinput_init(&events);
  }
  ConfigurationDescriptor.HID_ReportINEndpoint.PollingIntervalMS =
      config.pollrate;
  USB_Init();
  sei();
}

void output_tick(controller_t controller) {
  wdt_reset();
  // /* Must throw away unused bytes from the host, or it will lock up while
  //  * waiting for the device */
  int16_t b = CDC_Device_ReceiveByte(&SerialInterface);
  if (b > 0) { CDC_Device_SendByte(&SerialInterface, b); }
  if (b == 'b') { bootloader(); }
  if (b == 'r') {
    CDC_Device_SendData(&SerialInterface, &config, sizeof(config_t));
  }
  if (b == 'w') {
    uint8_t* data = &config;
    size_t i = 0;
    while (i < sizeof(config_t)) {
      data[i] = CDC_Device_ReceiveByte(&SerialInterface);
      if (data[i] != EOF) {
        i++;
      }
    }
    reboot();
  }
  CDC_Device_USBTask(&SerialInterface);
  events.tick(controller);
  USB_USBTask();
}
void EVENT_USB_Device_ConfigurationChanged(void) {
  events.configuration_changed();
  CDC_Device_ConfigureEndpoints(&SerialInterface);
}
void EVENT_USB_Device_ControlRequest(void) {
  events.control_request();
  CDC_Device_ProcessControlRequest(&SerialInterface);
}
void EVENT_USB_Device_StartOfFrame(void) { events.start_of_frame(); }

void CALLBACK_HID_Device_ProcessHIDReport(
    USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo, const uint8_t ReportID,
    const uint8_t ReportType, const void *ReportData,
    const uint16_t ReportSize) {
  events.process_hid_report(HIDInterfaceInfo, ReportID, ReportType, ReportData,
                            ReportSize);
}

bool CALLBACK_HID_Device_CreateHIDReport(
    USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo, uint8_t *const ReportID,
    const uint8_t ReportType, void *ReportData, uint16_t *const ReportSize) {
  return events.create_hid_report(HIDInterfaceInfo, ReportID, ReportType,
                                  ReportData, ReportSize);
}
/** CDC class driver callback function the processing of changes to the virtual
 *  control lines sent from the host..
 *
 *  \param[in] CDCInterfaceInfo  Pointer to the CDC class interface
 * configuration structure being referenced
 */
void EVENT_CDC_Device_ControLineStateChanged(
    USB_ClassInfo_CDC_Device_t *const CDCInterfaceInfo) {
  /* You can get changes to the virtual CDC lines in this callback; a common
     use-case is to use the Data Terminal Ready (DTR) flag to enable and
     disable CDC communications in your application when set to avoid the
     application blocking while waiting for a host to become ready and read
     in the pending data from the USB endpoints.
  */
  bool HostReady = (CDCInterfaceInfo->State.ControlLineStates.HostToDevice &
                    CDC_CONTROL_LINE_OUT_DTR) != 0;

  (void)HostReady;
}