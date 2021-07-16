#include "descriptors.h"

#include <stddef.h>

#include "usb/wcid_descriptors.h"

/** Language descriptor structure. This descriptor, located in FLASH memory, is
 * returned when the host requests the string descriptor with index 0 (the first
 * index). It is actually an array of 16-bit integers, which indicate via the
 * language ID table available at USB.org what languages the device supports for
 * its string descriptors.
 */
const TUSB_Descriptor_String_t languageString =
    TUSB_STRING_DESCRIPTOR_ARRAY(LANGUAGE_ID_ENG);
/** Manufacturer descriptor string. This is a Unicode string containing the
 * manufacturer's details in human readable form, and is read out upon request
 * by the host when the appropriate string ID is requested, listed in the Device
 *  Descriptor.
 */
const TUSB_Descriptor_String_t manufacturerString = {
    Header : {Size : sizeof(TUSB_Descriptor_Header_t) + (18),
              Type : TDTYPE_String},
    UnicodeString : {'s', 'a', 'n', 'j', 'a', 'y', '9', '0', '0'}
};
/** Product descriptor string. This is a Unicode string containing the product's
 * details in human readable form, and is read out upon request by the host when
 * the appropriate string ID is requested, listed in the Device Descriptor.
 */
const TUSB_Descriptor_String_t productString = {
    Header : {Size : sizeof(TUSB_Descriptor_Header_t) + (16),
              Type : TDTYPE_String},
    UnicodeString : {'A', 'r', 'd', 'w', 'i', 'i', 'n', 'o'}
};
/**
 * Descriptor used by the Xbox 360 to determine if a controller supports authentication
 */
const TUSB_Descriptor_String_t xboxString = {
    Header : {Size : sizeof(TUSB_Descriptor_Header_t) + (176),
              Type : TDTYPE_String},
    UnicodeString : {'X', 'b', 'o', 'x', ' ', 'S', 'e', 'c', 'u', 'r', 'i', 't', 'y', ' ', 'M', 'e', 't', 'h', 'o', 'd', ' ', '3', ',', ' ', 'V', 'e', 'r', 's', 'i', 'o', 'n', ' ', '1', '.', '0', '0', ',', ' ', 0xa9, ' ', '2', '0', '0', '5', ' ', 'M', 'i', 'c', 'r', 'o', 's', 'o', 'f', 't', ' ', 'C', 'o', 'r', 'p', 'o', 'r', 'a', 't', 'i', 'o', 'n', '.', ' ', 'A', 'l', 'l', ' ', 'r', 'i', 'g', 'h', 't', 's', ' ', 'r', 'e', 's', 'e', 'r', 'v', 'e', 'd', '.'}
};
const TUSB_Descriptor_String_t *const descriptorStrings[] = {
    &languageString, &manufacturerString, &productString};
/* A Microsoft-proprietary extension. String address 0xEE is used by
Windows for "OS Descriptors", which in this case allows us to indicate
that our device has a Compatible ID to provide. */
const TUSB_OSDescriptor_t OSDescriptorString = {
    Header : {Size : sizeof(TUSB_OSDescriptor_t), Type : TDTYPE_String},
    Signature : {'M', 'S', 'F', 'T', '1', '0', '0'},
    VendorCode : REQ_GET_OS_FEATURE_DESCRIPTOR,
    Reserved : 0
};

TUSB_Descriptor_Device_t deviceDescriptor = {
    Header : {Size : sizeof(deviceDescriptor), Type : TDTYPE_Device},
    USBSpecification : TVERSION_BCD(2, 0, 0),
    Class : 0xFF,
    SubClass : 0xFF,
    Protocol : 0xFF,
    Endpoint0Size : ENDPOINT_SIZE,
    VendorID : ARDWIINO_VID,
    ProductID : ARDWIINO_PID,
    ReleaseNumber : 0x0110,
    ManufacturerStrIndex : 0x01,
    ProductStrIndex : 0x02,
    SerialNumStrIndex : 0x03,
    NumberOfConfigurations : 1
};

TUSB_Descriptor_Configuration_XBOX_t XBOXConfigurationDescriptor = {
    Config : {Header : {Size : sizeof(TUSB_Descriptor_Configuration_Header_t),
                        Type : TDTYPE_Configuration},
              TotalConfigurationSize : sizeof(TUSB_Descriptor_Configuration_XBOX_t),
              TotalInterfaces : 4,
              ConfigurationNumber : 1,
              ConfigurationStrIndex : NO_DESCRIPTOR,
              ConfigAttributes :
                  (TUSB_CONFIG_TATTR_RESERVED | TUSB_CONFIG_TATTR_REMOTEWAKEUP),
              MaxPowerConsumption : TUSB_CONFIG_POWER_MA(500)},
    Interface1 : {Header : {Size : sizeof(XBOXConfigurationDescriptor.Interface1),
                            Type : TDTYPE_Interface},
                  InterfaceNumber : INTERFACE_ID_Device,
                  AlternateSetting : 0x00,
                  TotalEndpoints : 2,
                  Class : 0xff,
                  SubClass : 0x5d,
                  Protocol : 0x01,
                  InterfaceStrIndex : NO_DESCRIPTOR},

    Interface4 : {Header : {Size : sizeof(XBOXConfigurationDescriptor.Interface4),
                            Type : TDTYPE_Interface},
                  InterfaceNumber : INTERFACE_ID_XBOX_Security,
                  AlternateSetting : 0x00,
                  TotalEndpoints : 0,
                  Class : 0xff,
                  SubClass : 0xfd,
                  Protocol : 0x13,
                  InterfaceStrIndex : 4},
    InterfaceConfig : {
        Header : {Size : sizeof(TUSB_Descriptor_Interface_t), Type : TDTYPE_Interface},
        InterfaceNumber : INTERFACE_ID_Config,
        AlternateSetting : 0,
        TotalEndpoints : 0,
        Class : 0xff,
        SubClass : 0xff,
        Protocol : 0xff,
        InterfaceStrIndex : NO_DESCRIPTOR,
    },
    InterfaceExtra : {
        Header : {Size : sizeof(TUSB_Descriptor_Interface_t), Type : TDTYPE_Interface},
        InterfaceNumber : 2,
        AlternateSetting : 0,
        TotalEndpoints : 0,
        Class : 0xff,
        SubClass : 0xff,
        Protocol : 0xff,
        InterfaceStrIndex : NO_DESCRIPTOR,
    },
    Interface1ID : {
        Header : {Size : sizeof(XBOX_ID_Descriptor_t), Type : 0x21},
        reserved : {0x10, 0x01},
        subtype : 0x07,
        reserved2 : 0x25,
        bEndpointAddressIn : XINPUT_EPSIZE_IN,
        bMaxDataSizeIn : 0x14,
        reserved3 : {0x03, 0x03, 0x03, 0x04, 0x13},
        bEndpointAddressOut : XINPUT_EPSIZE_OUT,
        bMaxDataSizeOut : 0x08,
        reserved4 : {0x03, 0x03},
    },
    UnkownDescriptor4 : {0x06, 0x41, 0x00, 0x01, 0x01, 0x03},
    ReportINEndpoint11 : {
        Header : {Size : sizeof(XBOXConfigurationDescriptor.ReportINEndpoint11), Type : TDTYPE_Endpoint},
        EndpointAddress : DEVICE_EPADDR_IN,
        Attributes : (EP_TYPE_INTERRUPT | ENDPOINT_TATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        EndpointSize : XINPUT_EPSIZE_IN,
        PollingIntervalMS : 1,
    },
    ReportOUTEndpoint12 : {
        Header : {Size : sizeof(XBOXConfigurationDescriptor.ReportOUTEndpoint12), Type : TDTYPE_Endpoint},
        EndpointAddress : DEVICE_EPADDR_OUT,
        Attributes : (EP_TYPE_INTERRUPT | ENDPOINT_TATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        EndpointSize : XINPUT_EPSIZE_OUT,
        PollingIntervalMS : 1,
    },
};

TUSB_Descriptor_HID_Configuration_t HIDConfigurationDescriptor = {
    Config : {
        Header : {Size : sizeof(TUSB_Descriptor_Configuration_Header_t),
                  Type : TDTYPE_Configuration},
        TotalConfigurationSize : sizeof(TUSB_Descriptor_HID_Configuration_t),
        TotalInterfaces : 4,
        ConfigurationNumber : 1,
        ConfigurationStrIndex : NO_DESCRIPTOR,
        .ConfigAttributes =
            (TUSB_CONFIG_TATTR_RESERVED | TUSB_CONFIG_TATTR_REMOTEWAKEUP),
        MaxPowerConsumption : TUSB_CONFIG_POWER_MA(500),
    },
    InterfaceHID : {
        Header :
            {Size : sizeof(TUSB_Descriptor_Interface_t), Type : TDTYPE_Interface},
        InterfaceNumber : INTERFACE_ID_Device,
        AlternateSetting : 0,
        TotalEndpoints : 2,
        Class : THID_CSCP_HIDClass,
        SubClass : THID_CSCP_NonBootSubclass,
        Protocol : THID_CSCP_NonBootProtocol,
        InterfaceStrIndex : NO_DESCRIPTOR
    },
    EndpointInHID : {
        Header : {Size : sizeof(TUSB_Descriptor_Endpoint_t), Type : TDTYPE_Endpoint},
        EndpointAddress : DEVICE_EPADDR_IN,
        Attributes :
            (EP_TYPE_INTERRUPT | ENDPOINT_TATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        EndpointSize : XINPUT_EPSIZE_IN,
        PollingIntervalMS : 1
    },
    EndpointOutHID : {
        Header : {Size : sizeof(TUSB_Descriptor_Endpoint_t), Type : TDTYPE_Endpoint},
        EndpointAddress : DEVICE_EPADDR_OUT,
        Attributes :
            (EP_TYPE_INTERRUPT | ENDPOINT_TATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        EndpointSize : HID_EPSIZE_IN,
        PollingIntervalMS : 1
    },
    HIDDescriptor : {
        Header : {Size : sizeof(TUSB_THID_Descriptor_THID_t), Type : THID_DTYPE_HID},
        HIDSpec : TVERSION_BCD(1, 0, 1),
        CountryCode : 0x00,
        TotalReportDescriptors : 1,
        HIDReportType : THID_DTYPE_Report,
        HIDReportLength : 0
    },
    InterfaceConfig : {
        Header : {Size : sizeof(TUSB_Descriptor_Interface_t), Type : TDTYPE_Interface},
        InterfaceNumber : 1,
        AlternateSetting : 0,
        TotalEndpoints : 0,
        Class : 0xff,
        SubClass : 0xff,
        Protocol : 0xff,
        InterfaceStrIndex : NO_DESCRIPTOR,
    },
};

TUSB_Descriptor_MIDI_Configuration_t MIDIConfigurationDescriptor = {
    Config : {
        Header : {Size : sizeof(TUSB_Descriptor_Configuration_Header_t),
                  Type : TDTYPE_Configuration},
        TotalConfigurationSize : sizeof(TUSB_Descriptor_MIDI_Configuration_t),
        TotalInterfaces : 4,
        ConfigurationNumber : 1,
        ConfigurationStrIndex : NO_DESCRIPTOR,
        .ConfigAttributes =
            (TUSB_CONFIG_TATTR_RESERVED | TUSB_CONFIG_TATTR_REMOTEWAKEUP),
        MaxPowerConsumption : TUSB_CONFIG_POWER_MA(500),
    },
    Interface_AudioControl : {
        Header :
            {Size : sizeof(TUSB_Descriptor_Interface_t), Type : TDTYPE_Interface},
        InterfaceNumber : INTERFACE_ID_ControlStream,
        AlternateSetting : 0x00,
        TotalEndpoints : 0,
        Class : TAUDIO_CSCP_AudioClass,
        SubClass : TAUDIO_CSCP_ControlSubclass,
        Protocol : TAUDIO_CSCP_ControlProtocol,
        InterfaceStrIndex : NO_DESCRIPTOR
    },
    Interface_AudioStream : {
        Header :
            {Size : sizeof(TUSB_Descriptor_Interface_t), Type : TDTYPE_Interface},
        InterfaceNumber : INTERFACE_ID_AudioStream,
        AlternateSetting : 0x00,
        TotalEndpoints : 2,
        Class : TAUDIO_CSCP_AudioClass,
        SubClass : TAUDIO_CSCP_MIDIStreamingSubclass,
        Protocol : TAUDIO_CSCP_StreamingProtocol,
        InterfaceStrIndex : NO_DESCRIPTOR
    },
    Audio_ControlInterface_SPC : {
        Header : {
            Size : sizeof(TUSB_TAUDIO_Descriptor_Interface_AC_t),
            Type : TAUDIO_DTYPE_CSInterface
        },
        Subtype : TAUDIO_DSUBTYPE_CSInterface_Header,
        ACSpecification : TVERSION_BCD(1, 0, 0),
        TotalLength : sizeof(TUSB_TAUDIO_Descriptor_Interface_AC_t),
        InCollection : 1,
        InterfaceNumber : INTERFACE_ID_AudioStream,
    },
    Audio_StreamInterface_SPC : {
        Header : {
            Size : sizeof(TUSB_MIDI_Descriptor_AudioInterface_AS_t),
            Type : TAUDIO_DTYPE_CSInterface
        },
        Subtype : TAUDIO_DSUBTYPE_CSInterface_General,
        AudioSpecification : TVERSION_BCD(1, 0, 0),
        TotalLength :
            (sizeof(TUSB_Descriptor_MIDI_Configuration_t) -
             offsetof(TUSB_Descriptor_MIDI_Configuration_t, Audio_StreamInterface_SPC))
    },

    MIDI_In_Jack_Emb : {
        Header : {
            Size : sizeof(TUSB_MIDI_Descriptor_InputJack_t),
            Type : TAUDIO_DTYPE_CSInterface
        },
        Subtype : TAUDIO_DSUBTYPE_CSInterface_InputTerminal,
        JackType : TMIDI_JACKTYPE_Embedded,
        JackID : 0x01,
        JackStrIndex : NO_DESCRIPTOR
    },
    MIDI_In_Jack_Ext : {
        Header : {
            Size : sizeof(TUSB_MIDI_Descriptor_InputJack_t),
            Type : TAUDIO_DTYPE_CSInterface
        },
        Subtype : TAUDIO_DSUBTYPE_CSInterface_InputTerminal,
        JackType : TMIDI_JACKTYPE_External,
        JackID : 0x02,
        JackStrIndex : NO_DESCRIPTOR
    },
    MIDI_Out_Jack_Emb : {
        Header : {
            Size : sizeof(TUSB_MIDI_Descriptor_OutputJack_t),
            Type : TAUDIO_DTYPE_CSInterface
        },
        Subtype : TAUDIO_DSUBTYPE_CSInterface_OutputTerminal,
        JackType : TMIDI_JACKTYPE_Embedded,
        JackID : 0x03,
        NumberOfPins : 1,
        SourceJackID : {0x02},
        SourcePinID : {0x01},
        JackStrIndex : NO_DESCRIPTOR
    },
    MIDI_Out_Jack_Ext : {
        Header : {
            Size : sizeof(TUSB_MIDI_Descriptor_OutputJack_t),
            Type : TAUDIO_DTYPE_CSInterface
        },
        Subtype : TAUDIO_DSUBTYPE_CSInterface_OutputTerminal,
        JackType : TMIDI_JACKTYPE_External,
        JackID : 0x04,
        NumberOfPins : 1,
        SourceJackID : {0x01},
        SourcePinID : {0x01},
        JackStrIndex : NO_DESCRIPTOR
    },
    MIDI_Out_Jack_Endpoint : {
        Endpoint : {
            Header : {
                Size : sizeof(TUSB_TAUDIO_Descriptor_StreamEndpoint_Std_t),
                Type : TDTYPE_Endpoint
            },
            EndpointAddress : DEVICE_EPADDR_IN,
            Attributes : (EP_TYPE_BULK | ENDPOINT_TATTR_NO_SYNC |
                          ENDPOINT_USAGE_DATA),
            EndpointSize : HID_EPSIZE_IN,
            PollingIntervalMS : 1
        },
        Refresh : 0,
        SyncEndpointNumber : 0
    },
    MIDI_In_Jack_Endpoint_SPC : {
        Header : {
            Size : sizeof(TUSB_MIDI_Descriptor_Jack_Endpoint_t),
            Type : TAUDIO_DTYPE_CSEndpoint
        },
        Subtype : TAUDIO_DSUBTYPE_CSEndpoint_General,
        TotalEmbeddedJacks : 0x01,
        AssociatedJackID : {0x01}
    },
    MIDI_In_Jack_Endpoint : {
        Endpoint : {
            Header : {
                Size : sizeof(TUSB_TAUDIO_Descriptor_StreamEndpoint_Std_t),
                Type : TDTYPE_Endpoint
            },
            EndpointAddress : DEVICE_EPADDR_OUT,
            Attributes : (EP_TYPE_BULK | ENDPOINT_TATTR_NO_SYNC |
                          ENDPOINT_USAGE_DATA),
            EndpointSize : HID_EPSIZE_OUT,
            PollingIntervalMS : 1,
        },
        Refresh : 0,
        SyncEndpointNumber : 0
    },
    MIDI_Out_Jack_Endpoint_SPC : {
        Header : {
            Size : sizeof(TUSB_MIDI_Descriptor_Jack_Endpoint_t),
            Type : TAUDIO_DTYPE_CSEndpoint
        },
        Subtype : TAUDIO_DSUBTYPE_CSEndpoint_General,
        TotalEmbeddedJacks : 0x01,
        AssociatedJackID : {0x03}
    },
    InterfaceConfig : {
        Header : {Size : sizeof(TUSB_Descriptor_Interface_t), Type : TDTYPE_Interface},
        InterfaceNumber : 1,
        AlternateSetting : 0,
        TotalEndpoints : 0,
        Class : 0xff,
        SubClass : 0xff,
        Protocol : 0xff,
        InterfaceStrIndex : NO_DESCRIPTOR,
    },
};