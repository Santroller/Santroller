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
const PROGMEM USB_OSExtendedCompatibleIDDescriptor_t ExtendedIDs = {
  TotalLength : sizeof(USB_OSExtendedCompatibleIDDescriptor_t),
  Version : 0x0100,
  Index : EXTENDED_PROPERTIES_DESCRIPTOR,
  TotalSections : 1,
  SectionSize : 132,
  ExtendedID : {
    PropertyDataType : 1,
    PropertyNameLength : 40,
    PropertyName : L"DeviceInterfaceGUID",
    PropertyDataLength : 78,
    PropertyData : L"{DF59037D-7C92-4155-AC12-7D700A313D78}\0"
  }
};
const PROGMEM CompatibleDescriptorType DevCompatIDs = {
  TotalLength : sizeof(CompatibleDescriptorType),
  Version : 0x0100,
  Index : EXTENDED_COMPAT_ID_DESCRIPTOR,
#ifdef MULTI_ADAPTOR
  TotalSections : 4,
#else
  TotalSections : 2,
#endif
  Reserved : {0},
  CompatID : {
    FirstInterfaceNumber : INTERFACE_ID_XInput,
    Reserved : 0x04,
    CompatibleID : "XUSB10",
    SubCompatibleID : {0},
    Reserved2 : {0}
  },
  CompatID2 : {
    FirstInterfaceNumber : INTERFACE_ID_Config,
    Reserved : 0x04,
    CompatibleID : "WINUSB",
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
    processHIDReadFeatureReport();
  } else if (USB_ControlRequest.bRequest == HID_REQ_SetReport &&
             USB_ControlRequest.bmRequestType ==
                 (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE)) {
    Endpoint_ClearSETUP();
    Endpoint_Read_Control_Stream_LE(dbuf, USB_ControlRequest.wLength);
    Endpoint_ClearStatusStage();
    processHIDWriteFeatureReport(USB_ControlRequest.wLength, dbuf);
  } else if (USB_ControlRequest.bRequest == REQ_GetOSFeatureDescriptor &&
             (USB_ControlRequest.bmRequestType == 0xC1) &&
             USB_ControlRequest.wIndex == EXTENDED_PROPERTIES_DESCRIPTOR &&
             USB_ControlRequest.wValue == INTERFACE_ID_Config) {
    Endpoint_ClearSETUP();
    Endpoint_Write_Control_PStream_LE(&ExtendedIDs, ExtendedIDs.TotalLength);
    Endpoint_ClearStatusStage();
  } else if (USB_ControlRequest.bRequest == REQ_GetOSFeatureDescriptor &&
             USB_ControlRequest.bmRequestType ==
                 (REQDIR_DEVICETOHOST | REQTYPE_VENDOR) &&
             USB_ControlRequest.wIndex == EXTENDED_COMPAT_ID_DESCRIPTOR) {
    Endpoint_ClearSETUP();
    Endpoint_Write_Control_PStream_LE(&DevCompatIDs, DevCompatIDs.TotalLength);
    Endpoint_ClearStatusStage();
  } else if (USB_ControlRequest.bmRequestType ==
             (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE)) {
    Endpoint_ClearSETUP();
    Endpoint_ClearStatusStage();
  }
}
