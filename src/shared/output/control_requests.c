#include "control_requests.h"
#include "../config/defines.h"
#include "controller_structs.h"
#include "descriptors.h"
#include "output/serial_handler.h"
const USB_OSCompatibleIDDescriptor_t DevCompatIDs = {
  TotalLength : sizeof(USB_OSCompatibleIDDescriptor_t),
  Version : 0x0100,
  Index : EXTENDED_COMPAT_ID_DESCRIPTOR,
  TotalSections : 4,
  Reserved : {0},
  CompatID : {
    FirstInterfaceNumber : INTERFACE_ID_XInput,
    Reserved : 0x04,
    CompatibleID : "XUSB10",
    SubCompatibleID : {0},
    Reserved2 : {0}
  },
  CompatID2 : {
    FirstInterfaceNumber : INTERFACE_ID_XInput_2,
    Reserved : 0x04,
    CompatibleID : "XUSB10",
    SubCompatibleID : {0},
    Reserved2 : {0}
  },
  CompatID3 : {
    FirstInterfaceNumber : INTERFACE_ID_XInput_3,
    Reserved : 0x04,
    CompatibleID : "XUSB10",
    SubCompatibleID : {0},
    Reserved2 : {0}
  },
  CompatID4 : {
    FirstInterfaceNumber : INTERFACE_ID_XInput_4,
    Reserved : 0x04,
    CompatibleID : "XUSB10",
    SubCompatibleID : {0},
    Reserved2 : {0}
  }
};

void deviceControlRequest(void) {
  bool isWCID = USB_ControlRequest.bRequest == REQ_GetOSFeatureDescriptor &&
                USB_ControlRequest.bmRequestType ==
                    (REQDIR_DEVICETOHOST | REQTYPE_VENDOR) &&
                USB_ControlRequest.wIndex == EXTENDED_COMPAT_ID_DESCRIPTOR;
  if (!((USB_ControlRequest.bmRequestType ==
         (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE)) ||
        ((USB_ControlRequest.bmRequestType ==
              (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE) &&
          (USB_ControlRequest.bRequest == HID_REQ_GetReport))) ||
        isWCID))
    return;

  Endpoint_ClearSETUP();
  uint8_t reportID = (USB_ControlRequest.wValue & 0xFF);
  if (USB_ControlRequest.bRequest == HID_REQ_SetReport) {
    Endpoint_Read_Control_Stream_LE(dbuf, USB_ControlRequest.wLength);
    processHIDWriteFeatureReport(reportID, USB_ControlRequest.wLength, dbuf);
  } else if (USB_ControlRequest.bRequest == HID_REQ_GetReport) {
    processHIDReadFeatureReport(reportID);
  } else if (isWCID) {
    Endpoint_Write_Control_Stream_LE(&DevCompatIDs, DevCompatIDs.TotalLength);
  }
  Endpoint_ClearStatusStage();
}