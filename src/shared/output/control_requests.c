#include "control_requests.h"
#include "../config/defines.h"
#include "controller_structs.h"
#include "usb/Descriptors.h"
#include "usb/wcid.h"
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
static uint8_t id[] = {0x21, 0x26, 0x01, 0x07, 0x00, 0x00, 0x00, 0x00};
void controller_control_request(void) {
  if (device_type <= XINPUT_ARCADE_PAD_SUBTYPE) {
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
  if (device_type >= PS3_GAMEPAD_SUBTYPE) {
    if (device_type != SWITCH_GAMEPAD_SUBTYPE &&
        USB_ControlRequest.wIndex == INTERFACE_ID_HID) {
      if (USB_ControlRequest.bmRequestType ==
          (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE)) {
        if (USB_ControlRequest.bRequest == HID_REQ_GetReport) {
          // Is the id stuff below actually important? check with a ps3
          // emulator.
          if (device_type == PS3_GUITAR_GH_SUBTYPE) {
            id[3] = 0x06;
          } else if (device_type == PS3_GUITAR_RB_SUBTYPE) {
            id[3] = 0x00;
          } else if (device_type == PS3_DRUM_GH_SUBTYPE) {
            id[3] = 0x06;
          } else if (device_type == PS3_DRUM_RB_SUBTYPE) {
            id[3] = 0x00;
          }
          // Send out init packets for the ps3
          Endpoint_ClearSETUP();
          while (!(Endpoint_IsINReady()))
            ;
          for (uint8_t i = 0; i < sizeof(id); i++) { Endpoint_Write_8(id[i]); }
          Endpoint_ClearIN();
          Endpoint_ClearStatusStage();
          return;
        }
      }
    }
  }
}