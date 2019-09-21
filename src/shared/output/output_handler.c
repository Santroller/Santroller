#include "output_handler.h"
#include "avr/wdt.h"
#include "bootloader/bootloader.h"
#include "output_keyboard.h"
#include "output_ps3.h"
#include "output_serial.h"
#include "output_xinput.h"
#include "usb/Descriptors.h"

event_pointers events;
previous_buffer_t buffer;
USB_ClassInfo_HID_Device_t interface = {
  Config : {
    InterfaceNumber : INTERFACE_ID_HID,
    ReportINEndpoint : {
      Address : HID_EPADDR_IN,
      Size : HID_EPSIZE,
      Type : EP_TYPE_CONTROL,
      Banks : 1,
    },
    PrevReportINBuffer : &buffer,
    PrevReportINBufferSize : sizeof(buffer),
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
      .PollingIntervalMS = config.pollrate;
  if (config.sub_type >= KEYBOARD_SUBTYPE) {
    if (config.sub_type == KEYBOARD_SUBTYPE) {
      keyboard_init(&events, &interface);
    } else {
      ps3_init(&events, &interface);
    }
    hid_init();
  } else {
    xinput_init(&events, &interface);
  }

  USB_Init();
  sei();
}

void output_tick() {
  wdt_reset();
  HID_Device_USBTask(&interface);
}
void EVENT_USB_Device_ConfigurationChanged(void) {
  HID_Device_ConfigureEndpoints(&interface);
  USB_Device_EnableSOFEvents();
  serial_configuration_changed();
}
void EVENT_USB_Device_ControlRequest(void) {
  if (events.control_request) {
    events.control_request();
  } else {
    HID_Device_ProcessControlRequest(&interface);
  }
  serial_control_request();
}
void EVENT_USB_Device_StartOfFrame(void) {
  HID_Device_MillisecondElapsed(&interface);
}

bool CALLBACK_HID_Device_CreateHIDReport(
    USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo, uint8_t *const ReportID,
    const uint8_t ReportType, void *ReportData, uint16_t *const ReportSize) {
  return events.create_hid_report(HIDInterfaceInfo, ReportID, ReportType,
                                  ReportData, ReportSize);
}

void CALLBACK_HID_Device_ProcessHIDReport(
    USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo, const uint8_t ReportID,
    const uint8_t ReportType, const void *ReportData,
    const uint16_t ReportSize) {}
