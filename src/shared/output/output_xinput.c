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

static uint8_t prev_xinput_report[sizeof(USB_XInputReport_Data_t)];
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

const uint8_t PROGMEM dataLen8[] = {0x00, 0x08, 0x00, 0x00,
                                    0x00, 0x00, 0x00, 0x00};
const uint8_t PROGMEM deviceID[] = {0x00, 0x12, 0x28, 0x61};
const uint8_t PROGMEM dataLen20[] = {0x00, 0x14, 0x3f, 0xf7, 0xff, 0xff, 0x00,
                                     0x00, 0x00, 0x00, 0xc0, 0xff, 0xc0, 0xff,
                                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
void sendControl(const uint8_t *out, uint8_t outSize) {
  Endpoint_ClearStall();
  Endpoint_ClearSETUP();
  /* Write the report data to the control endpoint */
  Endpoint_Write_Control_Stream_LE(out, outSize);
  Endpoint_ClearOUT();
}
void xinput_control_request(void) {
  /* Handle HID Class specific requests */
  switch (USB_ControlRequest.bRequest) {
  case HID_REQ_GetReport:
    if (USB_ControlRequest.bmRequestType ==
        (REQDIR_DEVICETOHOST | REQTYPE_VENDOR | REQREC_INTERFACE)) {
      switch (USB_ControlRequest.wLength) {
      case 0x04:
        sendControl(deviceID, sizeof(deviceID));
        return;
      case 0x08:
        sendControl(dataLen8, sizeof(dataLen8));
        return;
      case 0x20:
        sendControl(dataLen20, sizeof(dataLen20));
        return;
      }
    }
    break;
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
  HID_Device_ProcessControlRequest(&interface);
}
void xinput_init(event_pointers *events,
                 USB_ClassInfo_HID_Device_t *hid_device) {
  events->create_hid_report = xinput_create_report;
  events->control_request = xinput_control_request;
  hid_device->Config.PrevReportINBuffer = &prev_xinput_report;
  hid_device->Config.PrevReportINBufferSize = sizeof(prev_xinput_report);
  ConfigurationDescriptor.Controller.XInput.XInputReserved.subtype =
      config.sub_type;
}