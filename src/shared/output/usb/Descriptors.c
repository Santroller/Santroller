#include "Descriptors.h"
#include "../../../shared/config/defines.h"
#include "../../../shared/output/reports.h"
#include "../../../shared/output/usb/wcid.h"
#include <LUFA/Drivers/USB/USB.h>
uint8_t device_type = OUTPUT_TYPE;
uint8_t polling_rate = POLL_RATE;
/** Language descriptor structure. This descriptor, located in FLASH memory, is
 * returned when the host requests the string descriptor with index 0 (the first
 * index). It is actually an array of 16-bit integers, which indicate via the
 * language ID table available at USB.org what languages the device supports for
 * its string descriptors.
 */
const USB_Descriptor_String_t PROGMEM LanguageString =
    USB_STRING_DESCRIPTOR_ARRAY(LANGUAGE_ID_ENG);

/** Manufacturer descriptor string. This is a Unicode string containing the
 * manufacturer's details in human readable form, and is read out upon request
 * by the host when the appropriate string ID is requested, listed in the Device
 *  Descriptor.
 */
const USB_Descriptor_String_t PROGMEM ManufacturerString =
    USB_STRING_DESCRIPTOR(L"sanjay900");

/** Product descriptor string. This is a Unicode string containing the product's
 * details in human readable form, and is read out upon request by the host when
 * the appropriate string ID is requested, listed in the Device Descriptor.
 */
const USB_Descriptor_String_t PROGMEM ProductString =
    USB_STRING_DESCRIPTOR(L"Ardwiino");

const USB_Descriptor_String_t PROGMEM VersionString =
    USB_STRING_DESCRIPTOR(L"" VERSION);

/* A Microsoft-proprietary extension. String address 0xEE is used by
Windows for "OS Descriptors", which in this case allows us to indicate
that our device has a Compatible ID to provide. */
const USB_OSDescriptor_t PROGMEM OSDescriptorString = {
  Header : {Size : sizeof(USB_OSDescriptor_t), Type : DTYPE_String},
  Signature : {'M', 'S', 'F', 'T', '1', '0', '0'},
  VendorCode : REQ_GetOSFeatureDescriptor,
  Reserved : 0
};

const USB_Descriptor_HIDReport_Datatype_t PROGMEM ps3_report_descriptor[] = {
    0x05, 0x01,       // Usage Page (Generic Desktop Ctrls)
    0x09, 0x05,       // Usage (Game Pad)
    0xA1, 0x01,       // Collection (Application)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x01,       //   Logical Maximum (1)
    0x35, 0x00,       //   Physical Minimum (0)
    0x45, 0x01,       //   Physical Maximum (1)
    0x75, 0x01,       //   Report Size (1)
    0x95, 0x0D,       //   Report Count (13)
    0x05, 0x09,       //   Usage Page (Button)
    0x19, 0x01,       //   Usage Minimum (0x01)
    0x29, 0x0D,       //   Usage Maximum (0x0D)
    0x81, 0x02,       //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No
                      //   Null Position)
    0x95, 0x03,       //   Report Count (3)
    0x81, 0x01,       //   Input (Const,Array,Abs,No Wrap,Linear,Preferred
                      //   State,No Null Position)
    0x05, 0x01,       //   Usage Page (Generic Desktop Ctrls)
    0x25, 0x07,       //   Logical Maximum (7)
    0x46, 0x3B, 0x01, //   Physical Maximum (315)
    0x75, 0x04,       //   Report Size (4)
    0x95, 0x01,       //   Report Count (1)
    0x65, 0x14,       //   Unit (System: English Rotation, Length: Centimeter)
    0x09, 0x39,       //   Usage (Hat switch)
    0x81, 0x42, //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,Null
                //   State)
    0x65, 0x00, //   Unit (None)
    0x95, 0x01, //   Report Count (1)
    0x81, 0x01, //   Input (Const,Array,Abs,No Wrap,Linear,Preferred
                //   State,No Null Position)
    0x26, 0xFF, 0x00, //   Logical Maximum (255)
    0x46, 0xFF, 0x00, //   Physical Maximum (255)
    0x09, 0x30,       //   Usage (X)
    0x09, 0x31,       //   Usage (Y)
    0x09, 0x32,       //   Usage (Z)
    0x09, 0x35,       //   Usage (Rz)
    0x75, 0x08,       //   Report Size (8)
    0x95, 0x04,       //   Report Count (4)
    0x81, 0x02,       //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No
                      //   Null Position)
    0x06, 0x00, 0xFF, //   Usage Page (Vendor Defined 0xFF00)
    0x09, 0x20,       //   Usage (0x20)
    0x09, 0x21,       //   Usage (0x21)
    0x09, 0x22,       //   Usage (0x22)
    0x09, 0x23,       //   Usage (0x23)
    0x09, 0x24,       //   Usage (0x24)
    0x09, 0x25,       //   Usage (0x25)
    0x09, 0x26,       //   Usage (0x26)
    0x09, 0x27,       //   Usage (0x27)
    0x09, 0x28,       //   Usage (0x28)
    0x09, 0x29,       //   Usage (0x29)
    0x09, 0x2A,       //   Usage (0x2A)
    0x09, 0x2B,       //   Usage (0x2B)
    0x95, 0x0C,       //   Report Count (12)
    0x81, 0x02,       //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No
                      //   Null Position)
    0x0A, 0x21, 0x26, //   Usage (0x2621)
    0x95, 0x08,       //   Report Count (8)
    0xB1, 0x02, //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No
                //   Null Position,Non-volatile)
    0x0A, 0x21, 0x26, //   Usage (0x2621)
    0x91, 0x02, //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No
                //   Null Position,Non-volatile)
    0x26, 0xFF, 0x03, //   Logical Maximum (1023)
    0x46, 0xFF, 0x03, //   Physical Maximum (1023)
    0x09, 0x2C,       //   Usage (0x2C)
    0x09, 0x2D,       //   Usage (0x2D)
    0x09, 0x2E,       //   Usage (0x2E)
    0x09, 0x2F,       //   Usage (0x2F)
    0x75, 0x10,       //   Report Size (16)
    0x95, 0x04,       //   Report Count (4)
    0x81, 0x02,       //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No
                      //   Null Position)
    0xC0,             // End Collection

};

const USB_Descriptor_HIDReport_Datatype_t PROGMEM
    keyboard_report_descriptor[] = {HID_DESCRIPTOR_KEYBOARD(SIMULTANEOUS_KEYS)};

const USB_Descriptor_Configuration_t PROGMEM ConfigurationDescriptor = {
  Config : {
    Header : {
      Size : sizeof(USB_Descriptor_Configuration_Header_t),
      Type : DTYPE_Configuration
    },

    TotalConfigurationSize : sizeof(USB_Descriptor_Configuration_t),
    TotalInterfaces : 3,

    ConfigurationNumber : 1,
    ConfigurationStrIndex : NO_DESCRIPTOR,

    ConfigAttributes : USB_CONFIG_ATTR_REMOTEWAKEUP,

    MaxPowerConsumption : USB_CONFIG_POWER_MA(250)
  },
  CDC_IAD : {
    Header : {
      Size : sizeof(USB_Descriptor_Interface_Association_t),
      Type : DTYPE_InterfaceAssociation
    },

    FirstInterfaceIndex : INTERFACE_ID_CDC_CCI,
    TotalInterfaces : 2,

    Class : CDC_CSCP_CDCClass,
    SubClass : CDC_CSCP_ACMSubclass,
    Protocol : CDC_CSCP_ATCommandProtocol,

    IADStrIndex : NO_DESCRIPTOR
  },

  CDC_CCI_Interface : {
    Header :
        {Size : sizeof(USB_Descriptor_Interface_t), Type : DTYPE_Interface},

    InterfaceNumber : INTERFACE_ID_CDC_CCI,
    AlternateSetting : 0,

    TotalEndpoints : 1,

    Class : CDC_CSCP_CDCClass,
    SubClass : CDC_CSCP_ACMSubclass,
    Protocol : CDC_CSCP_ATCommandProtocol,

    InterfaceStrIndex : NO_DESCRIPTOR
  },

  CDC_Functional_Header : {
    Header : {
      Size : sizeof(USB_CDC_Descriptor_FunctionalHeader_t),
      Type : CDC_DTYPE_CSInterface
    },
    Subtype : CDC_DSUBTYPE_CSInterface_Header,

    CDCSpecification : VERSION_BCD(1, 1, 0),
  },

  CDC_Functional_ACM : {
    Header : {
      Size : sizeof(USB_CDC_Descriptor_FunctionalACM_t),
      Type : CDC_DTYPE_CSInterface
    },
    Subtype : CDC_DSUBTYPE_CSInterface_ACM,

    Capabilities : 0x06,
  },

  CDC_Functional_Union : {
    Header : {
      Size : sizeof(USB_CDC_Descriptor_FunctionalUnion_t),
      Type : CDC_DTYPE_CSInterface
    },
    Subtype : CDC_DSUBTYPE_CSInterface_Union,

    MasterInterfaceNumber : INTERFACE_ID_CDC_CCI,
    SlaveInterfaceNumber : INTERFACE_ID_CDC_DCI,
  },

  CDC_NotificationEndpoint : {
    Header : {Size : sizeof(USB_Descriptor_Endpoint_t), Type : DTYPE_Endpoint},

    EndpointAddress : CDC_NOTIFICATION_EPADDR,
    Attributes :
        (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
    EndpointSize : CDC_NOTIFICATION_EPSIZE,
    PollingIntervalMS : 0xFF
  },

  CDC_DCI_Interface : {
    Header :
        {Size : sizeof(USB_Descriptor_Interface_t), Type : DTYPE_Interface},

    InterfaceNumber : INTERFACE_ID_CDC_DCI,
    AlternateSetting : 0,

    TotalEndpoints : 2,

    Class : CDC_CSCP_CDCDataClass,
    SubClass : CDC_CSCP_NoDataSubclass,
    Protocol : CDC_CSCP_NoDataProtocol,

    InterfaceStrIndex : NO_DESCRIPTOR
  },

  CDC_DataOutEndpoint : {
    Header : {Size : sizeof(USB_Descriptor_Endpoint_t), Type : DTYPE_Endpoint},

    EndpointAddress : CDC_RX_EPADDR,
    Attributes : (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
    EndpointSize : CDC_RX_EPSIZE,
    PollingIntervalMS : 0x05
  },

  CDC_DataInEndpoint : {
    Header : {Size : sizeof(USB_Descriptor_Endpoint_t), Type : DTYPE_Endpoint},

    EndpointAddress : CDC_TX_EPADDR,
    Attributes : (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
    EndpointSize : CDC_TX_EPSIZE,
    PollingIntervalMS : 0x05
  },
  Interface0 : {
    Header :
        {Size : sizeof(USB_Descriptor_Interface_t), Type : DTYPE_Interface},

    InterfaceNumber : 0,
    AlternateSetting : 0x00,

    TotalEndpoints : 2,

    Class : 0xFF,
    SubClass : 0x5D,
    Protocol : 0x01,

    InterfaceStrIndex : NO_DESCRIPTOR
  },
  Controller : {
    XInput : {
      XInputReserved : {
        Header : {Size : sizeof(USB_HID_XBOX_Descriptor_HID_t), Type : 0x21},
        {0x10, 0x01},
        0,
        {0x25, 0x81, 0x14, 0x03, 0x03, 0x03, 0x04, 0x13, 0x02, 0x08, 0x03, 0x03}
      },
      Endpoints : {
        DataInEndpoint0 : {
          Header :
              {Size : sizeof(USB_Descriptor_Endpoint_t), Type : DTYPE_Endpoint},

          EndpointAddress : 0x81,
          Attributes : EP_TYPE_INTERRUPT,
          EndpointSize : HID_EPSIZE,
          PollingIntervalMS : 1
        },
        DataOutEndpoint0 : {
          Header :
              {Size : sizeof(USB_Descriptor_Endpoint_t), Type : DTYPE_Endpoint},

          EndpointAddress : 0x02,
          Attributes : EP_TYPE_INTERRUPT,
          EndpointSize : HID_EPSIZE,
          PollingIntervalMS : 1
        },
      }
    }
  }
};
const USB_Descriptor_Device_t PROGMEM DeviceDescriptor = {
  Header : {Size : sizeof(USB_Descriptor_Device_t), Type : DTYPE_Device},

  USBSpecification : VERSION_BCD(2, 0, 0),
  Class : USB_CSCP_NoDeviceClass,
  SubClass : USB_CSCP_NoDeviceSubclass,
  Protocol : USB_CSCP_NoDeviceProtocol,
  Endpoint0Size : FIXED_CONTROL_ENDPOINT_SIZE,
  VendorID : 0x1209,
  ProductID : 0x2882,
  ReleaseNumber : 0x3122,

  ManufacturerStrIndex : 0x01,
  ProductStrIndex : 0x02,
  SerialNumStrIndex : 0x03,

  NumberOfConfigurations : FIXED_NUM_CONFIGURATIONS
};

const USB_HID_Descriptor_HID_t PROGMEM hid_descriptor = {
  Header : {Size : sizeof(USB_HID_Descriptor_HID_t), Type : HID_DTYPE_HID},

  HIDSpec : VERSION_BCD(1, 1, 1),
  CountryCode : 0x00,
  TotalReportDescriptors : 1,
  HIDReportType : HID_DTYPE_Report,
  HIDReportLength : 0
};

/** This function is called by the library when in device mode, and must be
 * overridden (see library "USB Descriptors" documentation) by the application
 * code so that the address and size of a requested descriptor can be given to
 * the USB library. When the device receives a Get Descriptor request on the
 * control endpoint, this function is called so that the descriptor details can
 * be passed back and the appropriate descriptor sent back to the USB host.
 */
uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                    const uint16_t wIndex,
                                    const void **const DescriptorAddress) {
  const uint8_t DescriptorType = (wValue >> 8);
  const uint8_t DescriptorNumber = (wValue & 0xFF);

  uint16_t Size = NO_DESCRIPTOR;

  const void *Address = NULL;
  uint8_t *buf = (uint8_t *)0x200;
  *DescriptorAddress = buf;
  // We set aside 0x200 as an area to work with descriptors.
  switch (DescriptorType) {
  case DTYPE_Device:
    Address = &DeviceDescriptor;
    Size = DeviceDescriptor.Header.Size;
    memcpy_P(buf, Address, Size);
    USB_Descriptor_Device_t *dev = (USB_Descriptor_Device_t *)buf;
    if (device_type == SWITCH_GAMEPAD) {
      dev->VendorID = 0x0F0D;
      dev->ProductID = 0x0092;
    } else {
      if (device_type > PS3_GAMEPAD) { dev->VendorID = 0x12ba; }
      if (device_type == PS3_GUITAR_HERO_GUITAR) {
        dev->ProductID = 0x0100;
      } else if (device_type == PS3_ROCK_BAND_GUITAR) {
        dev->ProductID = 0x0200;
      } else if (device_type == PS3_GUITAR_HERO_DRUMS) {
        dev->ProductID = 0x0120;
      } else if (device_type == PS3_ROCK_BAND_DRUMS) {
        dev->ProductID = 0x0210;
      }
    }
    return Size;
  case DTYPE_Configuration:
    Address = &ConfigurationDescriptor;
    Size = ConfigurationDescriptor.Config.TotalConfigurationSize;
    memcpy_P(buf, Address, Size);
    USB_Descriptor_Configuration_t *conf =
        (USB_Descriptor_Configuration_t *)buf;
    conf->Controller.XInput.Endpoints.DataInEndpoint0.PollingIntervalMS =
        polling_rate;
    if (device_type >= KEYBOARD) {
      // Switch from Xinput to HID descriptor layout
      memcpy_P(&conf->Controller.HID.Endpoints,
               &ConfigurationDescriptor.Controller.XInput.Endpoints,
               sizeof(conf->Controller.XInput.Endpoints));
      // And now adjust the total size as the HID layout is actually smaller
      conf->Config.TotalConfigurationSize -=
          sizeof(USB_HID_XBOX_Descriptor_HID_t) -
          sizeof(USB_HID_Descriptor_HID_t);

      memcpy_P(&conf->Controller.HID.HIDDescriptor, &hid_descriptor,
               sizeof(hid_descriptor));
      if (device_type == KEYBOARD) {
        conf->Controller.HID.HIDDescriptor.HIDReportLength =
            sizeof(keyboard_report_descriptor);
      } else {
        conf->Controller.HID.HIDDescriptor.HIDReportLength =
            sizeof(ps3_report_descriptor);
      }
      // Report that we have an HID device
      conf->Interface0.Class = HID_CSCP_HIDClass;
      conf->Interface0.SubClass = HID_CSCP_NonBootSubclass;
      conf->Interface0.Protocol = HID_CSCP_NonBootProtocol;
    }
    return Size;
  case HID_DTYPE_Report:
    if (device_type == KEYBOARD) {
      Address = keyboard_report_descriptor;
      Size = sizeof(keyboard_report_descriptor);
    } else {
      Address = ps3_report_descriptor;
      Size = sizeof(ps3_report_descriptor);
    }
    break;
  case DTYPE_String:
    switch (DescriptorNumber) {
    case 0x00:
      Address = &LanguageString;
      Size = LanguageString.Header.Size;
      break;
    case 0x01:
      Address = &ManufacturerString;
      Size = ManufacturerString.Header.Size;
      break;
    case 0x02:
      Address = &ProductString;
      Size = ProductString.Header.Size;
      break;
    case 0x03:
      Address = &VersionString;
      Size = VersionString.Header.Size;
      break;
    case 0xEE:
      Address = &OSDescriptorString;
      Size = OSDescriptorString.Header.Size;
      break;
    }
    break;
  }
  if (Size != NO_DESCRIPTOR) {
    memcpy_P(buf, Address, Size);
  } 
  return Size;
}
