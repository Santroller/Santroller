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
  if (deviceType <= XINPUT_ARCADE_PAD &&
      USB_ControlRequest.bRequest == REQ_GetOSFeatureDescriptor &&
      (USB_ControlRequest.bmRequestType &
       (CONTROL_REQTYPE_DIRECTION | CONTROL_REQTYPE_TYPE)) ==
          (REQDIR_DEVICETOHOST | REQTYPE_VENDOR) &&
      USB_ControlRequest.wIndex == EXTENDED_COMPAT_ID_DESCRIPTOR) {
    Endpoint_ClearSETUP();
    Endpoint_Write_Control_Stream_LE(&DevCompatIDs, DevCompatIDs.TotalLength);
    Endpoint_ClearStatusStage();
    return;
  }
#ifdef __AVR_ATmega16U2__
  if (deviceType < PS3_GAMEPAD || USB_ControlRequest.wIndex != INTERFACE_ID_HID)
    return;
#endif
  // // PS3 Id packet
  if (USB_ControlRequest.bmRequestType ==
          (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE) &&
      USB_ControlRequest.bRequest == HID_REQ_GetReport) {
    // Send out init packets for the ps3
    if (deviceType == PS3_GUITAR_HERO_GUITAR ||
        deviceType == PS3_GUITAR_HERO_DRUMS) {
      id[3] = 0x06;
    } else if (deviceType == PS3_ROCK_BAND_GUITAR ||
               deviceType == PS3_ROCK_BAND_DRUMS) {
      id[3] = 0x00;
    }
    Endpoint_ClearSETUP();
    Endpoint_Write_Control_Stream_LE(id, sizeof(id));
    Endpoint_ClearStatusStage();
    return;
  }
  // The following is necessary for wii rockband controllers. This implements
  // enough of the hid spec to make wii rockband controllers work
  if (USB_ControlRequest.bmRequestType !=
      (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE))
    return;

  Endpoint_ClearSETUP();
  if (USB_ControlRequest.bRequest == HID_REQ_SetReport) {
    Endpoint_Read_Control_Stream_LE(dbuf, USB_ControlRequest.wLength);
  }
  Endpoint_ClearStatusStage();
}