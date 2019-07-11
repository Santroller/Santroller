#include "output_handler.h"
#include "avr/wdt.h"
#include "bootloader/bootloader.h"
#include "output_hid.h"
#include "output_xinput.h"
#include "output_serial.h"
#include "usb/Descriptors.h"

event_pointers events;
void output_init(void) {
  ConfigurationDescriptor.Controller.XInput.Endpoints.DataInEndpoint0
      .PollingIntervalMS = config.pollrate;
  if (config.sub_type >= KEYBOARD_SUBTYPE) {
    hid_init(&events);
  } else {
    xinput_init(&events);
  }
  serial_init();
  USB_Init();
  sei();
}

void output_tick(controller_t controller) {
  wdt_reset();
  serial_tick();
  events.tick(controller);
  USB_USBTask();
}
void EVENT_USB_Device_ConfigurationChanged(void) {
  events.configuration_changed();
  serial_configuration_changed();
}
void EVENT_USB_Device_ControlRequest(void) {
  events.control_request();
  serial_control_request();
  
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
