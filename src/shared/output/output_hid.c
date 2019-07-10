#include "output_hid.h"
#include "avr/wdt.h"
#include "output_keyboard.h"
#include "output_ps3.h"
#include "output_switch.h"

controller_t last_controller;
USB_ClassInfo_HID_Device_t interface = {
  Config : {
    InterfaceNumber : INTERFACE_ID_HID,
    ReportINEndpoint : {
      Address : HID_EPADDR_IN,
      Size : HID_EPSIZE,
      Type : EP_TYPE_CONTROL,
      Banks : 1,
    },
    PrevReportINBuffer : NULL,
    PrevReportINBufferSize : 0,
  },
};


void hid_tick(controller_t controller) {
  memcpy(&last_controller, &controller, sizeof(controller_t));
  HID_Device_USBTask(&interface);
}
void hid_control_request(void) { HID_Device_ProcessControlRequest(&interface); }
void hid_configuration_changed(void) {
  HID_Device_ConfigureEndpoints(&interface);
  USB_Device_EnableSOFEvents();
}

void hid_start_of_frame(void) { HID_Device_MillisecondElapsed(&interface); }

void hid_process_report(USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo,
                        const uint8_t ReportID, const uint8_t ReportType,
                        const void *ReportData, const uint16_t ReportSize) {}
void hid_init(event_pointers *events) {
  events->tick = hid_tick;
  events->control_request = hid_control_request;
  events->start_of_frame = hid_start_of_frame;
  events->configuration_changed = hid_configuration_changed;
  events->process_hid_report = hid_process_report;
  if (config.sub_type == SWITCH_GAMEPAD_SUBTYPE) {
    switch_init(events, &interface);
  } else if (config.sub_type == KEYBOARD_SUBTYPE) {
    keyboard_init(events, &interface);
  } else if (config.sub_type >= PS3_GAMEPAD_SUBTYPE) {
    ps3_init(events, &interface);
  }
  // ConfigurationDescriptor.HID_GamepadHID.HIDReportLength = hid_report_size;
}