#include "output_xinput.h"
#include "output_handler.h"
#include "usb/Descriptors.h"
#include "usb/wcid.h"
#include <avr/wdt.h>
const USB_OSCompatibleIDDescriptor_t PROGMEM DevCompatIDs = {
  TotalLength : sizeof(USB_OSCompatibleIDDescriptor_t),
  Version : 0x0100,
  Index : EXTENDED_COMPAT_ID_DESCRIPTOR,
  TotalSections : 1,
  Reserved : {0},
  CompatID : {
    FirstInterfaceNumber : WCID_IF_NUMBER,
    Reserved : 0x04,
    CompatibleID : "XUSB10",
    SubCompatibleID : {0},
    Reserved2 : {0}
  }
};

void xinput_create_report(USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo,
                          uint8_t *const ReportID, const uint8_t ReportType,
                          void *ReportData, uint16_t *const ReportSize) {

  *ReportSize = sizeof(USB_XInputReport_Data_t);

  USB_XInputReport_Data_t *JoystickReport =
      (USB_XInputReport_Data_t *)ReportData;
  JoystickReport->rsize = sizeof(USB_XInputReport_Data_t);
  //Don't copy the extra device information tagged on the end of the controller.
  memcpy(&JoystickReport->buttons, &controller, sizeof(controller_t) - sizeof(uint16_t));
}
void xinput_control_request(void) {
  /* Handle HID Class specific requests */
  switch (USB_ControlRequest.bRequest) {
  case REQ_GetOSFeatureDescriptor:
    if ((USB_ControlRequest.bmRequestType &
         (CONTROL_REQTYPE_DIRECTION | CONTROL_REQTYPE_TYPE)) ==
            (REQDIR_DEVICETOHOST | REQTYPE_VENDOR) &&
        USB_ControlRequest.wIndex == EXTENDED_COMPAT_ID_DESCRIPTOR) {
      Endpoint_ClearSETUP();
      Endpoint_Write_Control_PStream_LE(&DevCompatIDs,
                                        DevCompatIDs.TotalLength);
      Endpoint_ClearOUT();
      return;
    }
  }
}
void xinput_init() {
  create_hid_report = xinput_create_report;
  control_request = xinput_control_request;
  ConfigurationDescriptor.Controller.XInput.XInputReserved.subtype =
      config.main.sub_type;
}