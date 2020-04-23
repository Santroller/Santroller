#include "control_requests.h"
#include "../config/defines.h"
#include "controller_structs.h"
#include "usb/Descriptors.h"
#include "usb/wcid.h"
const USB_OSCompatibleIDDescriptor_t DevCompatIDs = {
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
static uint8_t id[] = {0x21, 0x26, 0x01, 0x07, 0x00, 0x00, 0x00, 0x00};
void controller_control_request(void) {
  if (device_type <= XINPUT_ARCADE_PAD &&
      USB_ControlRequest.bRequest == REQ_GetOSFeatureDescriptor &&
      (USB_ControlRequest.bmRequestType &
       (CONTROL_REQTYPE_DIRECTION | CONTROL_REQTYPE_TYPE)) ==
          (REQDIR_DEVICETOHOST | REQTYPE_VENDOR) &&
      USB_ControlRequest.wIndex == EXTENDED_COMPAT_ID_DESCRIPTOR) {
    Endpoint_ClearSETUP();
    Endpoint_Write_Control_Stream_LE(&DevCompatIDs, DevCompatIDs.TotalLength);
    Endpoint_ClearOUT();
    return;
  }
  if (device_type >= PS3_GAMEPAD &&
      USB_ControlRequest.wIndex == INTERFACE_ID_HID &&
      USB_ControlRequest.bmRequestType ==
          (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE) &&
      USB_ControlRequest.bRequest == HID_REQ_GetReport) {
    // controller_control_request_init();
    // Send out init packets for the ps3
    Endpoint_ClearSETUP();
    Endpoint_Write_Control_Stream_LE(id, sizeof(id));
    Endpoint_ClearOUT();
    return;
  }
}