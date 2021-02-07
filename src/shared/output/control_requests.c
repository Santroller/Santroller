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
// Dumps from a real guitar
// const PROGMEM uint8_t c0w0000b1[] = {0x00, 0x82, 0xf8, 0x23};
const PROGMEM uint8_t c1w0000b1[] = {0x00, 0x08, 0x00, 0x00,
                                     0x00, 0x00, 0x00, 0x00};
// const PROGMEM uint8_t c1w0100b1[] = {0x00, 0x14, 0x3f, 0xf7, 0xff, 0xff,
// 0x00,
//                                      0x00, 0x00, 0x00, 0xc0, 0xff, 0xc0,
//                                      0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
//                                      0x00};

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
  const void *buffer = NULL;
  uint8_t len = 0;
  if (USB_ControlRequest.bRequest == HID_REQ_GetReport &&
      USB_ControlRequest.bmRequestType ==
          (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE)) {
    processHIDReadFeatureReport(USB_ControlRequest.wValue);
  } else if (USB_ControlRequest.bRequest == HID_REQ_SetReport &&
             USB_ControlRequest.bmRequestType ==
                 (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE)) {
    processHIDWriteFeatureReportControl(USB_ControlRequest.wValue,
                                        USB_ControlRequest.wLength);
  } else if (USB_ControlRequest.bRequest == 0x1 &&
             (USB_ControlRequest.bmRequestType == 0xC1) &&
             USB_ControlRequest.wIndex == INTERFACE_ID_XInput &&
             USB_ControlRequest.wValue == 0x0000) {
    len = sizeof(c1w0000b1);
    buffer = &c1w0000b1;
  } else if (USB_ControlRequest.bRequest == REQ_GetOSFeatureDescriptor &&
             (USB_ControlRequest.bmRequestType == 0xC1) &&
             USB_ControlRequest.wIndex == EXTENDED_PROPERTIES_DESCRIPTOR &&
             USB_ControlRequest.wValue == INTERFACE_ID_Config) {
    len = ExtendedIDs.TotalLength;
    buffer = &ExtendedIDs;
  } else if (USB_ControlRequest.bRequest == REQ_GetOSFeatureDescriptor &&
             USB_ControlRequest.bmRequestType ==
                 (REQDIR_DEVICETOHOST | REQTYPE_VENDOR) &&
             USB_ControlRequest.wIndex == EXTENDED_COMPAT_ID_DESCRIPTOR) {
    len = DevCompatIDs.TotalLength;
    buffer = &DevCompatIDs;
  } else if (USB_ControlRequest.bmRequestType ==
             (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE)) {
    buffer = &DevCompatIDs;
  }
  // Here are a couple of other control requests that are implemented, however
  // as we are running out of space and these were not required they are
  // disabled. 
  // } else if (USB_ControlRequest.bRequest == 0x1 &&
  //            (USB_ControlRequest.bmRequestType == 0xC0) &&
  //            USB_ControlRequest.wIndex == 0x00 &&
  //            USB_ControlRequest.wValue == 0x0000) {
  //   len = sizeof(c1w0000b1);
  //   buffer = &c0w0000b1;
  // } else if (USB_ControlRequest.bRequest == 0x1 &&
  //            (USB_ControlRequest.bmRequestType == 0xC1) &&
  //            USB_ControlRequest.wIndex == INTERFACE_ID_XInput &&
  //            USB_ControlRequest.wValue == 0x0100) {
  //   len = sizeof(c1w0100b1);
  //   buffer = &c1w0100b1;
  // }
  if (buffer) {
    Endpoint_ClearSETUP();
    Endpoint_Write_Control_PStream_LE(buffer, len);
    Endpoint_ClearStatusStage();
  }
}
