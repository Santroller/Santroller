#include "output_handler.h"
#include "avr/wdt.h"
#include "bootloader/bootloader.h"
#include "output_keyboard.h"
#include "output_ps3.h"
#include "output_serial.h"
#include "output_xinput.h"
#include "usb/Descriptors.h"
void (*control_request)(void);
void (*create_hid_report)(USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo,
                          uint8_t *const ReportID, const uint8_t ReportType,
                          void *ReportData, uint16_t *const ReportSize);
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
    PrevReportINBufferSize : sizeof(output_report_size_t),
  },
};

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
  // By not using the config here, we can get away with the UNO not having
  // access to the config.
  if (ConfigurationDescriptor.Controller.HID.HIDDescriptor.Header.Type ==
      0x29) { //0x29 - xinput reserved type
    // XINPUT
  } else if (ConfigurationDescriptor.Controller.HID.HIDDescriptor
                 .HIDReportLength == 137) { // ps3 descriptor has a size of 137
    // PS3

  } else {
    // KEYBOARD
  }
  if (control_request) { control_request(); }
  HID_Device_ProcessControlRequest(&interface);
  serial_control_request();
}
void EVENT_USB_Device_StartOfFrame(void) {
  HID_Device_MillisecondElapsed(&interface);
}

bool CALLBACK_HID_Device_CreateHIDReport(
    USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo, uint8_t *const ReportID,
    const uint8_t ReportType, void *ReportData, uint16_t *const ReportSize) {
  create_hid_report(HIDInterfaceInfo, ReportID, ReportType, ReportData,
                    ReportSize);
  return true;
}

void CALLBACK_HID_Device_ProcessHIDReport(
    USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo, const uint8_t ReportID,
    const uint8_t ReportType, const void *ReportData,
    const uint16_t ReportSize) {}