#include "descriptors.h"
#include "config/defines.h"
#include "controller/guitar_includes.h"
#include "output/reports.h"
#include "util/util.h"
#ifndef __AVR__
#  include "LUFAConfig.h"
#  include <tusb.h>
#endif
uint8_t deviceType = OUTPUT_TYPE;
uint8_t fullDeviceType = OUTPUT_TYPE;
/** Language descriptor structure. This descriptor, located in FLASH memory, is
 * returned when the host requests the string descriptor with index 0 (the first
 * index). It is actually an array of 16-bit integers, which indicate via the
 * language ID table available at USB.org what languages the device supports for
 * its string descriptors.
 */
AVR_CONST USB_Descriptor_String_t languageString =
    USB_STRING_DESCRIPTOR_ARRAY(LANGUAGE_ID_ENG);
/** Manufacturer descriptor string. This is a Unicode string containing the
 * manufacturer's details in human readable form, and is read out upon request
 * by the host when the appropriate string ID is requested, listed in the Device
 *  Descriptor.
 */
AVR_CONST USB_Descriptor_String_t manufacturerString = {
    .Header = {.Size = sizeof(USB_Descriptor_Header_t) + (18),
               .Type = DTYPE_String},
    .UnicodeString = {'s', 'a', 'n', 'j', 'a', 'y', '9', '0', '0'}};
/** Product descriptor string. This is a Unicode string containing the product's
 * details in human readable form, and is read out upon request by the host when
 * the appropriate string ID is requested, listed in the Device Descriptor.
 */
AVR_CONST USB_Descriptor_String_t productString = {
    .Header = {.Size = sizeof(USB_Descriptor_Header_t) + (16),
               .Type = DTYPE_String},
    .UnicodeString = {'A', 'r', 'd', 'w', 'i', 'i', 'n', 'o'}};
const USB_Descriptor_String_t *AVR_CONST descriptorStrings[] = {
    &languageString, &manufacturerString, &productString};
/* A Microsoft-proprietary extension. String address 0xEE is used by
Windows for "OS Descriptors", which in this case allows us to indicate
that our device has a Compatible ID to provide. */
AVR_CONST USB_OSDescriptor_t OSDescriptorString = {
  Header : {Size : sizeof(USB_OSDescriptor_t), Type : DTYPE_String},
  Signature : {'M', 'S', 'F', 'T', '1', '0', '0'},
  VendorCode : REQ_GetOSFeatureDescriptor,
  Reserved : 0
};
AVR_CONST USB_Descriptor_HIDReport_Datatype_t ps3_report_descriptor[] = {
    // Controller
    HID_RI_USAGE_PAGE(8, HID_USAGE_PAGE_GENERIC_DESKTOP),
    HID_RI_USAGE(8, HID_USAGE_GAMEPAD),
    HID_RI_COLLECTION(8, HID_COLLECTION_APPLICATION),
    HID_RI_LOGICAL_MINIMUM(8, 0),
    HID_RI_LOGICAL_MAXIMUM(8, 1),
    HID_RI_PHYSICAL_MINIMUM(8, 0),
    HID_RI_PHYSICAL_MAXIMUM(8, 1),
    HID_RI_REPORT_SIZE(8, 1),
    HID_RI_REPORT_COUNT(8, 13),
    HID_RI_USAGE_PAGE(8, HID_USAGE_PAGE_BUTTON),
    HID_RI_USAGE_MINIMUM(8, 0x01),
    HID_RI_USAGE_MAXIMUM(8, 0x0D),
    HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE |
                        HID_IOF_NO_WRAP | HID_IOF_LINEAR |
                        HID_IOF_PREFERRED_STATE | HID_IOF_NO_NULL_POSITION),
    HID_RI_REPORT_COUNT(8, 0x03),
    HID_RI_INPUT(8, HID_IOF_CONSTANT | HID_IOF_ARRAY | HID_IOF_ABSOLUTE |
                        HID_IOF_NO_WRAP | HID_IOF_LINEAR |
                        HID_IOF_PREFERRED_STATE | HID_IOF_NO_NULL_POSITION),
    HID_RI_USAGE_PAGE(8, HID_USAGE_PAGE_GENERIC_DESKTOP),
    HID_RI_LOGICAL_MAXIMUM(8, 7),
    HID_RI_PHYSICAL_MAXIMUM(16, 315),
    HID_RI_REPORT_SIZE(8, 4),
    HID_RI_REPORT_COUNT(8, 0x01),
    HID_RI_UNIT(8, HID_UNIT_DEGREES),
    HID_RI_USAGE(8, HID_USAGE_HAT_SWITCH),
    HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE |
                        HID_IOF_NO_WRAP | HID_IOF_LINEAR |
                        HID_IOF_PREFERRED_STATE | HID_IOF_NULLSTATE),
    HID_RI_UNIT(8, HID_UNIT_NONE),
    HID_RI_REPORT_COUNT(8, 0x01),
    HID_RI_INPUT(8, HID_IOF_CONSTANT | HID_IOF_ARRAY | HID_IOF_ABSOLUTE |
                        HID_IOF_NO_WRAP | HID_IOF_LINEAR |
                        HID_IOF_PREFERRED_STATE | HID_IOF_NO_NULL_POSITION),
    HID_RI_LOGICAL_MAXIMUM(16, 255),
    HID_RI_PHYSICAL_MAXIMUM(16, 255),
    HID_RI_USAGE(8, HID_USAGE_X),
    HID_RI_USAGE(8, HID_USAGE_Y),
    HID_RI_USAGE(8, HID_USAGE_Z),
    HID_RI_USAGE(8, HID_USAGE_Rz),
    HID_RI_REPORT_SIZE(8, 8),
    HID_RI_REPORT_COUNT(8, 4),
    HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE |
                        HID_IOF_NO_WRAP | HID_IOF_LINEAR |
                        HID_IOF_PREFERRED_STATE | HID_IOF_NO_NULL_POSITION),
    HID_RI_USAGE_PAGE(16, 0xFF00),
    HID_RI_USAGE(8, 0x20),
    HID_RI_USAGE(8, 0x21),
    HID_RI_USAGE(8, 0x22),
    HID_RI_USAGE(8, 0x23),
    HID_RI_USAGE(8, 0x24),
    HID_RI_USAGE(8, 0x25),
    HID_RI_USAGE(8, 0x26),
    HID_RI_USAGE(8, 0x27),
    HID_RI_USAGE(8, 0x28),
    HID_RI_USAGE(8, 0x29),
    HID_RI_USAGE(8, 0x2A),
    HID_RI_USAGE(8, 0x2B),
    HID_RI_REPORT_COUNT(8, 12),
    HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE |
                        HID_IOF_NO_WRAP | HID_IOF_LINEAR |
                        HID_IOF_PREFERRED_STATE | HID_IOF_NO_NULL_POSITION),
    HID_RI_USAGE(16, 0x2621),
    HID_RI_REPORT_COUNT(8, 32),
    HID_RI_FEATURE(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE |
                          HID_IOF_NO_WRAP | HID_IOF_LINEAR |
                          HID_IOF_PREFERRED_STATE | HID_IOF_NO_NULL_POSITION |
                          HID_IOF_NON_VOLATILE),
    HID_RI_USAGE(16, 0x2621),
    HID_RI_OUTPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE |
                         HID_IOF_NO_WRAP | HID_IOF_LINEAR |
                         HID_IOF_PREFERRED_STATE | HID_IOF_NO_NULL_POSITION |
                         HID_IOF_NON_VOLATILE),
    HID_RI_LOGICAL_MAXIMUM(16, 1023),
    HID_RI_PHYSICAL_MAXIMUM(16, 1023),
    HID_RI_USAGE(8, 0x2C),
    HID_RI_USAGE(8, 0x2D),
    HID_RI_USAGE(8, 0x2E),
    HID_RI_USAGE(8, 0x2F),
    HID_RI_REPORT_SIZE(8, 16),
    HID_RI_REPORT_COUNT(8, 4),
    HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE |
                        HID_IOF_NO_WRAP | HID_IOF_LINEAR |
                        HID_IOF_PREFERRED_STATE | HID_IOF_NO_NULL_POSITION |
                        HID_IOF_NON_VOLATILE),
    HID_RI_END_COLLECTION(0),
};

#define Buttons 4
#define MinAxisVal -127
#define MaxAxisVal 127
#define MinPhysicalVal -127
#define MaxPhysicalVal 128
#define AbsoluteCoords false
AVR_CONST USB_Descriptor_HIDReport_Datatype_t kbd_report_descriptor[] = {
    HID_RI_USAGE_PAGE(8, HID_USAGE_PAGE_GENERIC_DESKTOP),
    HID_RI_USAGE(8, HID_USAGE_KEYBOARD),
    HID_RI_COLLECTION(8, HID_COLLECTION_APPLICATION),
    HID_RI_REPORT_ID(8, REPORT_ID_KBD),
    HID_RI_USAGE_PAGE(8, HID_USAGE_PAGE_KEYBOARD),
    HID_RI_USAGE_MINIMUM(8, 0xE0),
    HID_RI_USAGE_MAXIMUM(8, 0xE7),
    HID_RI_LOGICAL_MINIMUM(8, 0x00),
    HID_RI_LOGICAL_MAXIMUM(8, 0x01),
    HID_RI_REPORT_SIZE(8, 0x01),
    HID_RI_REPORT_COUNT(8, 0x08),
    HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),
    HID_RI_REPORT_COUNT(8, 0x01),
    HID_RI_REPORT_SIZE(8, 0x08),
    HID_RI_INPUT(8, HID_IOF_CONSTANT),
    HID_RI_USAGE_PAGE(8, HID_USAGE_PAGE_LED),
    HID_RI_USAGE_MINIMUM(8, 0x01),
    HID_RI_USAGE_MAXIMUM(8, 0x05),
    HID_RI_REPORT_COUNT(8, 0x05),
    HID_RI_REPORT_SIZE(8, 0x01),
    HID_RI_OUTPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE |
                         HID_IOF_NON_VOLATILE),
    HID_RI_REPORT_COUNT(8, 0x01),
    HID_RI_REPORT_SIZE(8, 0x03),
    HID_RI_OUTPUT(8, HID_IOF_CONSTANT),
    HID_RI_LOGICAL_MINIMUM(8, 0x00),
    HID_RI_LOGICAL_MAXIMUM(16, 0xFF),
    HID_RI_USAGE_PAGE(8, 0x07),
    HID_RI_USAGE_MINIMUM(8, 0x00),
    HID_RI_USAGE_MAXIMUM(8, 0xFF),
    HID_RI_REPORT_COUNT(8, SIMULTANEOUS_KEYS),
    HID_RI_REPORT_SIZE(8, 0x08),
    HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_ARRAY | HID_IOF_ABSOLUTE),
    HID_RI_END_COLLECTION(0),
    HID_RI_USAGE_PAGE(8, HID_USAGE_PAGE_GENERIC_DESKTOP),
    HID_RI_USAGE(8, HID_USAGE_MOUSE),
    HID_RI_COLLECTION(8, HID_COLLECTION_APPLICATION),
    HID_RI_REPORT_ID(8, REPORT_ID_MOUSE),
    HID_RI_USAGE(8, HID_USAGE_POINTER),
    HID_RI_COLLECTION(8, HID_COLLECTION_PHYSICAL),
    HID_RI_USAGE_PAGE(8, HID_USAGE_PAGE_BUTTON),
    HID_RI_USAGE_MINIMUM(8, 0x01),
    HID_RI_USAGE_MAXIMUM(8, Buttons),
    HID_RI_LOGICAL_MINIMUM(8, 0x00),
    HID_RI_LOGICAL_MAXIMUM(8, 0x01),
    HID_RI_REPORT_COUNT(8, Buttons),
    HID_RI_REPORT_SIZE(8, 0x01),
    HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),
    HID_RI_REPORT_SIZE(8, (8 - (Buttons % 8))),
    HID_RI_REPORT_COUNT(8, 0x01),
    HID_RI_INPUT(8, HID_IOF_CONSTANT | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),
    HID_RI_USAGE_PAGE(8, HID_USAGE_PAGE_GENERIC_DESKTOP),
    HID_RI_USAGE(8, HID_USAGE_X),
    HID_RI_USAGE(8, HID_USAGE_Y),
    HID_RI_USAGE(8, HID_USAGE_Wheel),
    HID_RI_LOGICAL_MINIMUM(8, -127),
    HID_RI_LOGICAL_MAXIMUM(8, 127),
    HID_RI_REPORT_COUNT(8, 0x03),
    HID_RI_REPORT_SIZE(8, 8),
    HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE |
                        (AbsoluteCoords ? HID_IOF_ABSOLUTE : HID_IOF_RELATIVE)),
    HID_RI_USAGE_PAGE(8, HID_USAGE_PAGE_CONSUMER),
    HID_RI_USAGE(16, HID_USAGE_CONSUMER_AC_PAN),
    HID_RI_LOGICAL_MINIMUM(16, -127),
    HID_RI_LOGICAL_MAXIMUM(16, 127),
    HID_RI_REPORT_COUNT(8, 0x01),
    HID_RI_REPORT_SIZE(8, 8),
    HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_RELATIVE),
    HID_RI_END_COLLECTION(0),
    HID_RI_END_COLLECTION(0),
};

#define ARDWIINO_VID 0x1209
#define ARDWIINO_PID 0x2882
AVR_CONST USB_Descriptor_Device_t deviceDescriptor = {
  Header : {Size : sizeof(USB_Descriptor_Device_t), Type : DTYPE_Device},
  USBSpecification : VERSION_BCD(2, 0, 0),
  Class : USB_CSCP_NoDeviceClass,
  SubClass : USB_CSCP_NoDeviceSubclass,
  Protocol : USB_CSCP_NoDeviceProtocol,
  Endpoint0Size : FIXED_CONTROL_ENDPOINT_SIZE,
  VendorID : ARDWIINO_VID,
  ProductID : ARDWIINO_PID,
  ReleaseNumber : VERSION_BCD(VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION),
  ManufacturerStrIndex : 0x01,
  ProductStrIndex : 0x02,
  SerialNumStrIndex : USE_INTERNAL_SERIAL,
  NumberOfConfigurations : FIXED_NUM_CONFIGURATIONS
};
AVR_CONST USB_Descriptor_Configuration_t ConfigurationDescriptor = {
  Config : {
    Header : {
      Size : sizeof(USB_Descriptor_Configuration_Header_t),
      Type : DTYPE_Configuration
    },
    TotalConfigurationSize : sizeof(USB_Descriptor_Configuration_t),
    TotalInterfaces : 5,
    ConfigurationNumber : 1,
    ConfigurationStrIndex : NO_DESCRIPTOR,
    ConfigAttributes : USB_CONFIG_ATTR_RESERVED,
    MaxPowerConsumption : USB_CONFIG_POWER_MA(100)
  },
  InterfaceXInput : {
    Header :
        {Size : sizeof(USB_Descriptor_Interface_t), Type : DTYPE_Interface},
    InterfaceNumber : INTERFACE_ID_XInput,
    AlternateSetting : 0,
    TotalEndpoints : 2,
    Class : 0xFF,
    SubClass : 0x5D,
    Protocol : 0x01,
    InterfaceStrIndex : NO_DESCRIPTOR
  },
  XInputReserved : {
    Header : {Size : sizeof(USB_HID_XBOX_Descriptor_HID_t), Type : 0x21},
    reserved : {0x00, 0x01},
    subtype : 0x00,
    reserved2 : 0x25,
    bEndpointAddressIn : XINPUT_EPADDR_IN,
    bMaxDataSizeIn : HID_EPSIZE,
    reserved3 : {0x00, 0x00, 0x00, 0x00, 0x13},
    bEndpointAddressOut : XINPUT_EPADDR_OUT,
    bMaxDataSizeOut : HID_EPSIZE,
    reserved4 : {0x00, 0x00}
  },
  EndpointInXInput : {
    Header : {Size : sizeof(USB_Descriptor_Endpoint_t), Type : DTYPE_Endpoint},
    EndpointAddress : XINPUT_EPADDR_IN,
    Attributes : EP_TYPE_INTERRUPT,
    EndpointSize : HID_EPSIZE,
    PollingIntervalMS : 1
  },
  EndpointOutXInput : {
    Header : {Size : sizeof(USB_Descriptor_Endpoint_t), Type : DTYPE_Endpoint},
    EndpointAddress : XINPUT_EPADDR_OUT,
    Attributes : EP_TYPE_INTERRUPT,
    EndpointSize : HID_EPSIZE,
    PollingIntervalMS : 1
  },

  Interface_AudioControl : {
    Header :
        {Size : sizeof(USB_Descriptor_Interface_t), Type : DTYPE_Interface},
    InterfaceNumber : INTERFACE_ID_ControlStream,
    AlternateSetting : 0x00,
    TotalEndpoints : 0,
    Class : AUDIO_CSCP_AudioClass,
    SubClass : AUDIO_CSCP_ControlSubclass,
    Protocol : AUDIO_CSCP_ControlProtocol,
    InterfaceStrIndex : NO_DESCRIPTOR
  },
  Interface_AudioStream : {
    Header :
        {Size : sizeof(USB_Descriptor_Interface_t), Type : DTYPE_Interface},
    InterfaceNumber : INTERFACE_ID_AudioStream,
    AlternateSetting : 0x00,
    TotalEndpoints : 2,
    Class : AUDIO_CSCP_AudioClass,
    SubClass : AUDIO_CSCP_MIDIStreamingSubclass,
    Protocol : AUDIO_CSCP_StreamingProtocol,
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
  MIDI_Out_Jack_Endpoint : {
    Endpoint : {
      Header : {
        Size : sizeof(USB_Audio_Descriptor_StreamEndpoint_Std_t),
        Type : DTYPE_Endpoint
      },
      EndpointAddress : MIDI_EPADDR_IN,
      Attributes : (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
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
  MIDI_In_Jack_Endpoint : {
    Endpoint : {
      Header : {
        Size : sizeof(USB_Audio_Descriptor_StreamEndpoint_Std_t),
        Type : DTYPE_Endpoint
      },
      EndpointAddress : MIDI_EPADDR_OUT,
      Attributes : (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
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
  InterfaceConfig : {
    Header :
        {Size : sizeof(USB_Descriptor_Interface_t), Type : DTYPE_Interface},
    InterfaceNumber : INTERFACE_ID_Config,
    AlternateSetting : 0,
    TotalEndpoints : 0,
    Class : 0xff,
    SubClass : 0xff,
    Protocol : 0xff,
    InterfaceStrIndex : NO_DESCRIPTOR
  },
  InterfaceHID : {
    Header :
        {Size : sizeof(USB_Descriptor_Interface_t), Type : DTYPE_Interface},
    InterfaceNumber : INTERFACE_ID_HID,
    AlternateSetting : 0,
    TotalEndpoints : 2,
    Class : HID_CSCP_HIDClass,
    SubClass : HID_CSCP_NonBootSubclass,
    Protocol : HID_CSCP_NonBootProtocol,
    InterfaceStrIndex : NO_DESCRIPTOR
  },

  EndpointInHID : {
    Header : {Size : sizeof(USB_Descriptor_Endpoint_t), Type : DTYPE_Endpoint},
    EndpointAddress : HID_EPADDR_IN,
    Attributes :
        (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
    EndpointSize : HID_EPSIZE,
    PollingIntervalMS : 1
  },
  EndpointOutHID : {
    Header : {Size : sizeof(USB_Descriptor_Endpoint_t), Type : DTYPE_Endpoint},
    EndpointAddress : HID_EPADDR_OUT,
    Attributes :
        (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
    EndpointSize : HID_EPSIZE,
    PollingIntervalMS : 1
  },
  HIDDescriptor : {
    Header : {Size : sizeof(USB_HID_Descriptor_HID_t), Type : HID_DTYPE_HID},
    HIDSpec : VERSION_BCD(1, 0, 1),
    CountryCode : 0x00,
    TotalReportDescriptors : 1,
    HIDReportType : HID_DTYPE_Report,
    HIDReportLength : sizeof(ps3_report_descriptor)
  },
};
AVR_CONST uint16_t vid[] = {0x0F0D,       0x12ba, 0x12ba, 0x12ba, 0x12ba,
                            ARDWIINO_VID, 0x12ba, 0x12ba, 0x1bad, 0x1bad};
AVR_CONST uint16_t pid[] = {0x0092,       0x0100, 0x0120, 0x0200, 0x0210,
                            ARDWIINO_PID, 0x0140, 0x074b, 0x0004, 0x0005};