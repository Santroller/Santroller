#include "output_init.h"
USB_HID_Descriptor_HID_t hid_descriptor = {
  Header : {Size : sizeof(USB_HID_Descriptor_HID_t), Type : HID_DTYPE_HID},

  HIDSpec : VERSION_BCD(1, 1, 1),
  CountryCode : 0x00,
  TotalReportDescriptors : 1,
  HIDReportType : HID_DTYPE_Report,
  HIDReportLength : 0
};
void hid_init(void) {
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
void output_init(void) {
  ConfigurationDescriptor.Controller.XInput.Endpoints.DataInEndpoint0
      .PollingIntervalMS = config.main.poll_rate;
  if (config.main.sub_type >= KEYBOARD_SUBTYPE) {
    if (config.main.sub_type == KEYBOARD_SUBTYPE) {
      keyboard_init();
    } else {
      ps3_init();
    }
    hid_init();
  } else {
    xinput_init();
  }

  USB_Init();
  sei();
}