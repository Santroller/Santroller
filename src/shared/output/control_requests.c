#include "control_requests.h"
#include "../config/defines.h"
#include "controller_structs.h"
#include "descriptors.h"
#include "output/serial_handler.h"
#ifdef MULTI_ADAPTOR
#define CompatibleDescriptorType USB_OSCompatibleIDDescriptor_4_t
#else
#define CompatibleDescriptorType USB_OSCompatibleIDDescriptor_t
#endif
const CompatibleDescriptorType DevCompatIDs = {
  TotalLength : sizeof(CompatibleDescriptorType),
  Version : 0x0100,
  Index : EXTENDED_COMPAT_ID_DESCRIPTOR,
#ifdef MULTI_ADAPTOR
  TotalSections : 4,
#else
  TotalSections : 1,
#endif
  Reserved : {0},
  CompatID : {
    FirstInterfaceNumber : INTERFACE_ID_XInput,
    Reserved : 0x04,
    CompatibleID : "XUSB10",
    SubCompatibleID : {0},
    Reserved2 : {0}
  },
#ifdef MULTI_ADAPTOR
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
#endif
};

void deviceControlRequest(void) {
  bool isWCID = USB_ControlRequest.bRequest == REQ_GetOSFeatureDescriptor &&
                USB_ControlRequest.bmRequestType ==
                    (REQDIR_DEVICETOHOST | REQTYPE_VENDOR) &&
                USB_ControlRequest.wIndex == EXTENDED_COMPAT_ID_DESCRIPTOR;
  bool isGetReport =
      USB_ControlRequest.bmRequestType ==
          (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE) &&
      (USB_ControlRequest.bRequest == HID_REQ_GetReport);
  if (!((USB_ControlRequest.bmRequestType ==
         (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE)) ||
        isGetReport || isWCID))
    return;

  Endpoint_ClearSETUP();
  uint8_t reportID = (USB_ControlRequest.wValue & 0xFF);
  if (USB_ControlRequest.bRequest == HID_REQ_SetReport) {
    Endpoint_Read_Control_Stream_LE(dbuf, USB_ControlRequest.wLength);
    processHIDWriteFeatureReport(reportID, USB_ControlRequest.wLength, dbuf);
  } else if (isGetReport) {
    processHIDReadFeatureReport(reportID);
  } else if (isWCID) {
    Endpoint_Write_Control_Stream_LE(&DevCompatIDs, DevCompatIDs.TotalLength);
  }
  Endpoint_ClearStatusStage();
}