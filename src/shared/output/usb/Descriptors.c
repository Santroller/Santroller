#include "Descriptors.h"
#include "../../../shared/config/defines.h"
#include "../../../shared/output/reports.h"
#include "../../../shared/output/usb/wcid.h"
uint8_t device_type = OUTPUT_TYPE;
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

const USB_Descriptor_String_t *const PROGMEM Strings[] = {
    &LanguageString, &ManufacturerString, &ProductString, &VersionString};
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
    TotalInterfaces : 4,

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
  Interface_AudioControl : {
    Header :
        {Size : sizeof(USB_Descriptor_Interface_t), Type : DTYPE_Interface},

    InterfaceNumber : INTERFACE_ID_Unused,
    AlternateSetting : 0x00,

    TotalEndpoints : 0,

    Class : AUDIO_CSCP_AudioClass,
    SubClass : AUDIO_CSCP_ControlSubclass,
    Protocol : AUDIO_CSCP_ControlProtocol,

    InterfaceStrIndex : NO_DESCRIPTOR
  },
  Audio_ControlInterface_SPC : {
    Header : {
      Size : sizeof(USB_Audio_Descriptor_Interface_AC_t),
      Type : AUDIO_DTYPE_CSInterface
    },
    Subtype : AUDIO_DSUBTYPE_CSInterface_Header,

    ACSpecification : VERSION_BCD(1, 0, 0),
    TotalLength : sizeof(USB_Audio_Descriptor_Interface_AC_t),

    InCollection : 1,
    InterfaceNumber : INTERFACE_ID_AudioStream,
  },

  Interface0 : {
    Header :
        {Size : sizeof(USB_Descriptor_Interface_t), Type : DTYPE_Interface},

    InterfaceNumber : INTERFACE_ID_HID,
    AlternateSetting : 0,

    TotalEndpoints : 2,

    Class : 0xFF,
    SubClass : 0x5D,
    Protocol : 0x01,

    InterfaceStrIndex : NO_DESCRIPTOR
  },
  Audio_StreamInterface_SPC : {
    Header : {
      Size : sizeof(USB_MIDI_Descriptor_AudioInterface_AS_t),
      Type : AUDIO_DTYPE_CSInterface
    },
    Subtype : AUDIO_DSUBTYPE_CSInterface_General,

    AudioSpecification : VERSION_BCD(1, 0, 0),

    TotalLength :
        (sizeof(USB_Descriptor_Configuration_t) -
         offsetof(USB_Descriptor_Configuration_t, Audio_StreamInterface_SPC))
  },
  XInputReserved : {
    Header : {Size : sizeof(USB_HID_XBOX_Descriptor_HID_t), Type : 0x21},
    {0x10, 0x01},
    0,
    {0x25, 0x81, 0x14, 0x03, 0x03, 0x03, 0x04, 0x13, 0x02, 0x08, 0x03, 0x03}
  },
  HIDDescriptor : {
    Header : {Size : sizeof(USB_HID_Descriptor_HID_t), Type : DTYPE_Other},

    HIDSpec : VERSION_BCD(1, 1, 1),
    CountryCode : 0x00,
    TotalReportDescriptors : 1,
    HIDReportType : HID_DTYPE_Report,
    HIDReportLength : sizeof(ps3_report_descriptor)
  },

  MIDI_In_Jack_Emb : {
    Header : {
      Size : sizeof(USB_MIDI_Descriptor_InputJack_t),
      Type : AUDIO_DTYPE_CSInterface
    },
    Subtype : AUDIO_DSUBTYPE_CSInterface_InputTerminal,

    JackType : MIDI_JACKTYPE_Embedded,
    JackID : 0x01,

    JackStrIndex : NO_DESCRIPTOR
  },

  MIDI_In_Jack_Ext : {
    Header : {
      Size : sizeof(USB_MIDI_Descriptor_InputJack_t),
      Type : AUDIO_DTYPE_CSInterface
    },
    Subtype : AUDIO_DSUBTYPE_CSInterface_InputTerminal,

    JackType : MIDI_JACKTYPE_External,
    JackID : 0x02,

    JackStrIndex : NO_DESCRIPTOR
  },

  MIDI_Out_Jack_Emb : {
    Header : {
      Size : sizeof(USB_MIDI_Descriptor_OutputJack_t),
      Type : AUDIO_DTYPE_CSInterface
    },
    Subtype : AUDIO_DSUBTYPE_CSInterface_OutputTerminal,

    JackType : MIDI_JACKTYPE_Embedded,
    JackID : 0x03,

    NumberOfPins : 1,
    SourceJackID : {0x02},
    SourcePinID : {0x01},

    JackStrIndex : NO_DESCRIPTOR
  },

  MIDI_Out_Jack_Ext : {
    Header : {
      Size : sizeof(USB_MIDI_Descriptor_OutputJack_t),
      Type : AUDIO_DTYPE_CSInterface
    },
    Subtype : AUDIO_DSUBTYPE_CSInterface_OutputTerminal,

    JackType : MIDI_JACKTYPE_External,
    JackID : 0x04,

    NumberOfPins : 1,
    SourceJackID : {0x01},
    SourcePinID : {0x01},

    JackStrIndex : NO_DESCRIPTOR
  },
  DataInEndpoint0 : {
    Endpoint : {
      Header : {
        Size : sizeof(USB_Audio_Descriptor_StreamEndpoint_Std_t),
        Type : DTYPE_Endpoint
      },

      EndpointAddress : HID_EPADDR_IN,
      Attributes : EP_TYPE_INTERRUPT,
      EndpointSize : HID_EPSIZE,
      PollingIntervalMS : 1
    },
    Refresh : 0,
    SyncEndpointNumber : 0
  },

  MIDI_In_Jack_Endpoint_SPC : {
    Header : {
      Size : sizeof(USB_MIDI_Descriptor_Jack_Endpoint_t),
      Type : AUDIO_DTYPE_CSEndpoint
    },
    Subtype : AUDIO_DSUBTYPE_CSEndpoint_General,

    TotalEmbeddedJacks : 0x01,
    AssociatedJackID : {0x01}
  },
  DataOutEndpoint0 : {
    Endpoint : {
      Header : {
        Size : sizeof(USB_Audio_Descriptor_StreamEndpoint_Std_t),
        Type : DTYPE_Endpoint
      },

      EndpointAddress : HID_EPADDR_OUT,
      Attributes : EP_TYPE_INTERRUPT,
      EndpointSize : HID_EPSIZE,
      PollingIntervalMS : 1,
    },
    Refresh : 0,
    SyncEndpointNumber : 0
  },

  MIDI_Out_Jack_Endpoint_SPC : {
    Header : {
      Size : sizeof(USB_MIDI_Descriptor_Jack_Endpoint_t),
      Type : AUDIO_DTYPE_CSEndpoint
    },
    Subtype : AUDIO_DSUBTYPE_CSEndpoint_General,

    TotalEmbeddedJacks : 0x01,
    AssociatedJackID : {0x03}
  },
};
#define ARDWIINO_VID 0x1209
#define ARDWIINO_PID 0x2882
const USB_Descriptor_Device_t PROGMEM DeviceDescriptor = {
  Header : {Size : sizeof(USB_Descriptor_Device_t), Type : DTYPE_Device},

  USBSpecification : VERSION_BCD(2, 0, 0),
  Class : USB_CSCP_NoDeviceClass,
  SubClass : USB_CSCP_NoDeviceSubclass,
  Protocol : USB_CSCP_NoDeviceProtocol,
  Endpoint0Size : FIXED_CONTROL_ENDPOINT_SIZE,
  VendorID : ARDWIINO_VID,
  ProductID : ARDWIINO_PID,
  ReleaseNumber : 0x3122,

  ManufacturerStrIndex : 0x01,
  ProductStrIndex : 0x02,
  SerialNumStrIndex : 0x03,

  NumberOfConfigurations : FIXED_NUM_CONFIGURATIONS
};

uint8_t dbuf[sizeof(USB_Descriptor_Configuration_t)];
const uint16_t PROGMEM vid[] = {0x0F0D, ARDWIINO_VID, 0x12ba, 0x12ba,
                                0x12ba, 0x12ba,       0x1bad, 0x1bad};
const uint16_t PROGMEM pid[] = {0x0092, ARDWIINO_PID, 0x0100, 0x0200,
                                0x0120, 0x0210,       0x0004, 0x074B};
#define MIDI_SIZE                                                              \
  sizeof(USB_Audio_Descriptor_Interface_AC_t) +                                \
      sizeof(USB_Descriptor_Interface_t) +                                     \
      sizeof(USB_MIDI_Descriptor_AudioInterface_AS_t) +                        \
      sizeof(USB_MIDI_Descriptor_InputJack_t) +                                \
      sizeof(USB_MIDI_Descriptor_InputJack_t) +                                \
      sizeof(USB_MIDI_Descriptor_OutputJack_t) +                               \
      sizeof(USB_MIDI_Descriptor_OutputJack_t)
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
  *DescriptorAddress = dbuf;
  switch (DescriptorType) {
  case DTYPE_Device:
    Address = &DeviceDescriptor;
    Size = DeviceDescriptor.Header.Size;
    memcpy_P(dbuf, Address, Size);
    USB_Descriptor_Device_t *dev = (USB_Descriptor_Device_t *)dbuf;
    if (device_type >= SWITCH_GAMEPAD) {
      uint8_t offs = device_type - SWITCH_GAMEPAD;
      dev->VendorID = pgm_read_word(vid + offs);
      dev->ProductID = pgm_read_word(pid + offs);
    }
    return Size;
  case DTYPE_Configuration:
    Address = &ConfigurationDescriptor;
    Size = ConfigurationDescriptor.Config.TotalConfigurationSize;
    memcpy_P(dbuf, Address, Size);
    USB_Descriptor_Configuration_t *conf =
        (USB_Descriptor_Configuration_t *)dbuf;
    if (device_type >= MIDI_GUITAR) {
      conf->Interface0.Class = AUDIO_CSCP_AudioClass;
      conf->Interface0.SubClass = AUDIO_CSCP_MIDIStreamingSubclass;
      conf->Interface0.Protocol = AUDIO_CSCP_StreamingProtocol;
      conf->DataOutEndpoint0.Endpoint.Attributes =
          (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA);
      conf->DataInEndpoint0.Endpoint.Attributes =
          (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA);
      conf->Interface_AudioControl.InterfaceNumber = INTERFACE_ID_ControlStream;
      conf->Interface0.InterfaceNumber = INTERFACE_ID_AudioStream;
      // We need to skip over the HID Descriptor and the XInputReserved descriptor. Treating them as part of the interface desrcriptor does this nicely.
      conf->Interface0.Header.Size = sizeof(USB_Descriptor_Interface_t) + sizeof(USB_HID_Descriptor_HID_t) + sizeof(USB_HID_XBOX_Descriptor_HID_t);
    } else if (device_type >= KEYBOARD) {
      if (device_type == KEYBOARD) {
        conf->HIDDescriptor.HIDReportLength =
            sizeof(keyboard_report_descriptor);
      }
      // Report that we have an HID device
      conf->Interface0.Class = HID_CSCP_HIDClass;
      conf->Interface0.SubClass = HID_CSCP_NonBootSubclass;
      conf->Interface0.Protocol = HID_CSCP_NonBootProtocol;
      conf->HIDDescriptor.Header.Type = HID_DTYPE_HID;
    } else {
      // Map fake subtypes to their real counterparts
      uint8_t st = device_type;
      switch (st) {
      case XINPUT_ROCK_BAND_DRUMS:
      case XINPUT_GUITAR_HERO_DRUMS:
        st = REAL_DRUM_SUBTYPE;
        break;
      case XINPUT_LIVE_GUITAR:
      case XINPUT_GUITAR_HERO_GUITAR:
      case XINPUT_ROCK_BAND_GUITAR:
        st = REAL_GUITAR_SUBTYPE;
        break;
      }
      conf->XInputReserved.subtype = st;
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
    if (DescriptorNumber <= 3) {
      Address = (void *)pgm_read_word(Strings + DescriptorNumber);
    } else if (DescriptorNumber == 0xEE) {
      Address = &OSDescriptorString;
    } else {
      break;
    }
    Size =
        pgm_read_byte(Address + offsetof(USB_StdDescriptor_String_t, bLength));
    break;
  }
  if (Size != NO_DESCRIPTOR) { memcpy_P(dbuf, Address, Size); }
  return Size;
}
