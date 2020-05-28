#include "control_requests.h"
#include "../config/defines.h"
#include "controller_structs.h"
#include "descriptors.h"
const USB_OSCompatibleIDDescriptor_t DevCompatIDs = {
  TotalLength : sizeof(USB_OSCompatibleIDDescriptor_t),
  Version : 0x0100,
  Index : EXTENDED_COMPAT_ID_DESCRIPTOR,
  TotalSections : 1,
  Reserved : {0},
  CompatID : {
    FirstInterfaceNumber : INTERFACE_ID_HID,
    Reserved : 0x04,
    CompatibleID : "XUSB10",
    SubCompatibleID : {0},
    Reserved2 : {0}
  }
};
static uint8_t id[] = {0x21, 0x26, 0x01, 0x07, 0x00, 0x00, 0x00, 0x00};
extern uint8_t dbuf[sizeof(USB_Descriptor_Configuration_t)];
void deviceControlRequest(void) {
  // This is awful, but to save code space on the 8u2, we flatten all of this into a single call to setup and clearstatuspage.
  // This frees up an insane amount of bytes compared to writing this like serial is done in EVENT_USB_Device_ControlRequest
#ifdef __AVR_ATmega32U4__
  if (!((deviceType >= SWITCH_GAMEPAD && USB_ControlRequest.wIndex == INTERFACE_ID_HID) ||
      USB_ControlRequest.wIndex == EXTENDED_COMPAT_ID_DESCRIPTOR))
    return;
#endif
  // Half implement HID as the wii requires it. Also handle PS3 id packets and xbox 360 id packets here too.
  if (!(USB_ControlRequest.bmRequestType ==
            (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE) ||
        (USB_ControlRequest.bmRequestType ==
            (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE) && USB_ControlRequest.bRequest == HID_REQ_SetReport) ||
        USB_ControlRequest.bRequest == REQ_GetOSFeatureDescriptor))
    return;
// Something here is responding when it shouldnt, so we should work out exactly what that is.
  Endpoint_ClearSETUP();
  if (USB_ControlRequest.bRequest == HID_REQ_GetReport) {
    // PS3 Id packet
    if (deviceType <= PS3_GUITAR_HERO_DRUMS) {
      id[3] = 0x06;
    } else if (deviceType <= PS3_ROCK_BAND_DRUMS) {
      id[3] = 0x00;
    }
    Endpoint_Write_Control_Stream_LE(id, sizeof(id));
  } else if (USB_ControlRequest.bRequest == HID_REQ_SetReport) {
    Endpoint_Read_Control_Stream_LE(dbuf, USB_ControlRequest.wLength);
  } else if (USB_ControlRequest.bRequest == REQ_GetOSFeatureDescriptor) {
    Endpoint_Write_Control_Stream_LE(&DevCompatIDs, DevCompatIDs.TotalLength);
  }
  Endpoint_ClearStatusStage();
}