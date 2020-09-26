#include "control_requests.h"
#include "../config/defines.h"
#include "controller_structs.h"
#include "descriptors.h"
#include "output/serial_handler.h"
#ifdef MULTI_ADAPTOR
#  define CompatibleDescriptorType USB_OSCompatibleIDDescriptor_4_t
#else
#  define CompatibleDescriptorType USB_OSCompatibleIDDescriptor_t
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
  if (!(Endpoint_IsSETUPReceived())) return;
  if (USB_ControlRequest.bRequest == HID_REQ_GetReport &&
      USB_ControlRequest.bmRequestType ==
          (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE)) {
    Endpoint_ClearSETUP();
    processHIDReadFeatureReport();
  } else if (USB_ControlRequest.bRequest == HID_REQ_SetReport &&
             USB_ControlRequest.bmRequestType ==
                 (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE)) {
    Endpoint_ClearSETUP();
    Endpoint_Read_Control_Stream_LE(dbuf, USB_ControlRequest.wLength);
    Endpoint_ClearStatusStage();
    processHIDWriteFeatureReport(USB_ControlRequest.wLength, dbuf);
  } else if (USB_ControlRequest.bRequest == REQ_GetOSFeatureDescriptor &&
             USB_ControlRequest.bmRequestType ==
                 (REQDIR_DEVICETOHOST | REQTYPE_VENDOR) &&
             USB_ControlRequest.wIndex == EXTENDED_COMPAT_ID_DESCRIPTOR) {
    Endpoint_ClearSETUP();
    Endpoint_Write_Control_Stream_LE(&DevCompatIDs, DevCompatIDs.TotalLength);
    Endpoint_ClearStatusStage();
  } else if (USB_ControlRequest.bmRequestType ==
             (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE)) {
    Endpoint_ClearSETUP();
    Endpoint_ClearStatusStage();
  }
}
