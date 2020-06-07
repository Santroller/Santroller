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
    FirstInterfaceNumber : INTERFACE_ID_XInput,
    Reserved : 0x04,
    CompatibleID : "XUSB10",
    SubCompatibleID : {0},
    Reserved2 : {0}
  }
};
static uint8_t id[] = {0x21, 0x26, 0x01, 0x07, 0x00, 0x00, 0x00, 0x00};
extern uint8_t dbuf[sizeof(USB_Descriptor_Configuration_t)];
// For space saving reasons, its easier to define this in here with ifdefs
// instead of it being in uno/usb/main.c
#ifdef __AVR_ATmega16U2__
/** Contains the current baud rate and other settings of the first virtual
 * serial port. This must be retained as some operating systems will not open
 * the port unless the settings can be set successfully.
 */
CDC_LineEncoding_t lineEncoding = {0};
extern bool avrdudeInUse;
extern void setDeviceMode(bool);
#  include <LUFA/Drivers/Board/Board.h>
#endif
void deviceControlRequest(void) {
  // Both WCID and CDC_REQ_SetLineEncoding use the same bRequest. The difference
  // is the bmRequestType, so we can be more specific for WCID and less specific
  // for serial. This is only an issue on the uno, the micro handles serial
  // using LUFA
  bool isXinput = deviceType <= XINPUT_ARCADE_PAD &&
                  USB_ControlRequest.bRequest == REQ_GetOSFeatureDescriptor &&
                  USB_ControlRequest.bmRequestType ==
                      (REQDIR_DEVICETOHOST | REQTYPE_VENDOR) &&
                  USB_ControlRequest.wIndex == EXTENDED_COMPAT_ID_DESCRIPTOR;
  // Implement all necessary control requests for hid and serial (only on uno).
  // The micro has enough space to implement serial properley, so in that case
  // we use lufas implementation.
  if (!((USB_ControlRequest.bmRequestType ==
         (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE)) ||
        ((USB_ControlRequest.bmRequestType ==
              (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE) &&
          (USB_ControlRequest.bRequest == HID_REQ_GetReport ||
           USB_ControlRequest.bRequest == CDC_REQ_GetLineEncoding))) ||
        isXinput))
    return;

  Endpoint_ClearSETUP();
  if (USB_ControlRequest.bRequest == HID_REQ_SetReport) {
    Endpoint_Read_Control_Stream_LE(dbuf, USB_ControlRequest.wLength);
  } else if (USB_ControlRequest.bRequest == HID_REQ_GetReport) {
    if (deviceType <= PS3_ROCK_BAND_DRUMS) {
      id[3] = 0x00;
    } else if (deviceType <= PS3_GUITAR_HERO_DRUMS) {
      id[3] = 0x00;
    }
    // Send out init packets for the ps3
    Endpoint_Write_Control_Stream_LE(id, sizeof(id));
  } else if (isXinput) {
    Endpoint_Write_Control_Stream_LE(&DevCompatIDs, DevCompatIDs.TotalLength);
  }
#ifdef __AVR_ATmega16U2__
  else if (USB_ControlRequest.bRequest == CDC_REQ_GetLineEncoding) {
    Endpoint_Write_Control_Stream_LE(&lineEncoding, sizeof(CDC_LineEncoding_t));
  } else if (USB_ControlRequest.bRequest == CDC_REQ_SetLineEncoding) {
    Endpoint_Read_Control_Stream_LE(&lineEncoding, sizeof(CDC_LineEncoding_t));
  } else if (USB_ControlRequest.bRequest == CDC_REQ_SetControlLineState) {
    if (USB_ControlRequest.wValue & CDC_CONTROL_LINE_OUT_DTR)
      AVR_RESET_LINE_PORT &= ~AVR_RESET_LINE_MASK;
    else
      AVR_RESET_LINE_PORT |= AVR_RESET_LINE_MASK;
    if (avrdudeInUse) {
      avrdudeInUse = false;
      setDeviceMode(true);
    }
  }
#endif
  Endpoint_ClearStatusStage();
}