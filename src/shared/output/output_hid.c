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

USB_HID_Descriptor_HID_t hid_descriptor = {
  Header : {Size : sizeof(USB_HID_Descriptor_HID_t), Type : HID_DTYPE_HID},

  HIDSpec : VERSION_BCD(1, 1, 1),
  CountryCode : 0x00,
  TotalReportDescriptors : 1,
  HIDReportType : HID_DTYPE_Report,
  HIDReportLength : 0
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
  if (config.sub_type == KEYBOARD_SUBTYPE) {
    keyboard_init(events, &interface);
  } else {
    ps3_init(events, &interface);
  }
  // if (config.sub_type == SWITCH_GAMEPAD_SUBTYPE) {
  //   switch_init(events, &interface);
  // }  
  // Swap from XInput layout to HID layout
  memmove(&ConfigurationDescriptor.Controller.HID.Endpoints,
          &ConfigurationDescriptor.Controller.XInput.Endpoints,
          sizeof(ConfigurationDescriptor.Controller.XInput.Endpoints));
  // And now adjust the total size as the HID layout is actually smaller
  ConfigurationDescriptor.Config.TotalConfigurationSize -=
      sizeof(USB_HID_XBOX_Descriptor_HID_t) - sizeof(USB_HID_Descriptor_HID_t);

  hid_descriptor.HIDReportLength = hid_report_size;
  ConfigurationDescriptor.Controller.HID.HIDDescriptor = hid_descriptor;
  // Report that we have an HID device
  ConfigurationDescriptor.Interface0.Class = HID_CSCP_HIDClass;
  ConfigurationDescriptor.Interface0.SubClass = HID_CSCP_NonBootSubclass;
  ConfigurationDescriptor.Interface0.Protocol = HID_CSCP_NonBootProtocol;
}