#include "descriptors.h"

#include <stddef.h>

#include "wcid_descriptors.h"

/** Language descriptor structure. This descriptor, located in FLASH memory, is
 * returned when the host requests the string descriptor with index 0 (the first
 * index). It is actually an array of 16-bit integers, which indicate via the
 * language ID table available at USB.org what languages the device supports for
 * its string descriptors.
 */
const USB_Descriptor_String_t languageString =
    USB_STRING_DESCRIPTOR_ARRAY(LANGUAGE_ID_ENG);
/** Manufacturer descriptor string. This is a Unicode string containing the
 * manufacturer's details in human readable form, and is read out upon request
 * by the host when the appropriate string ID is requested, listed in the Device
 *  Descriptor.
 */
const USB_Descriptor_String_t manufacturerString = {
    Header : {Size : sizeof(USB_Descriptor_Header_t) + (18),
              Type : DTYPE_String},
    UnicodeString : {'s', 'a', 'n', 'j', 'a', 'y', '9', '0', '0'}
};
/** Product descriptor string. This is a Unicode string containing the product's
 * details in human readable form, and is read out upon request by the host when
 * the appropriate string ID is requested, listed in the Device Descriptor.
 */
const USB_Descriptor_String_t productString = {
    Header : {Size : sizeof(USB_Descriptor_Header_t) + (16),
              Type : DTYPE_String},
    UnicodeString : {'A', 'r', 'd', 'w', 'i', 'i', 'n', 'o'}
};
/**
 * Descriptor used by the Xbox 360 to determine if a controller supports authentication
 */
const USB_Descriptor_String_t xboxString = {
    Header : {Size : sizeof(USB_Descriptor_Header_t) + (176),
              Type : DTYPE_String},
    UnicodeString : {'X', 'b', 'o', 'x', ' ', 'S', 'e', 'c', 'u', 'r', 'i', 't', 'y', ' ', 'M', 'e', 't', 'h', 'o', 'd', ' ', '3', ',', ' ', 'V', 'e', 'r', 's', 'i', 'o', 'n', ' ', '1', '.', '0', '0', ',', ' ', 0xa9, ' ', '2', '0', '0', '5', ' ', 'M', 'i', 'c', 'r', 'o', 's', 'o', 'f', 't', ' ', 'C', 'o', 'r', 'p', 'o', 'r', 'a', 't', 'i', 'o', 'n', '.', ' ', 'A', 'l', 'l', ' ', 'r', 'i', 'g', 'h', 't', 's', ' ', 'r', 'e', 's', 'e', 'r', 'v', 'e', 'd', '.'}
};
const USB_Descriptor_String_t *const descriptorStrings[] = {
    &languageString, &manufacturerString, &productString};
/* A Microsoft-proprietary extension. String address 0xEE is used by
Windows for "OS Descriptors", which in this case allows us to indicate
that our device has a Compatible ID to provide. */
const USB_OSDescriptor_t OSDescriptorString = {
    Header : {Size : sizeof(USB_OSDescriptor_t), Type : DTYPE_String},
    Signature : {'M', 'S', 'F', 'T', '1', '0', '0'},
    VendorCode : REQ_GET_OS_FEATURE_DESCRIPTOR,
    Reserved : 0
};

USB_Descriptor_Device_t deviceDescriptor = {
    Header : {Size : sizeof(deviceDescriptor), Type : DTYPE_Device},
    USBSpecification : VERSION_BCD(2, 0, 0),
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

USB_Descriptor_Configuration_XBOX_t XBOXConfigurationDescriptor = {
    Config : {Header : {Size : sizeof(USB_Descriptor_Configuration_Header_t),
                        Type : DTYPE_Configuration},
              TotalConfigurationSize : sizeof(USB_Descriptor_Configuration_XBOX_t),
              TotalInterfaces : 4,
              ConfigurationNumber : 1,
              ConfigurationStrIndex : NO_DESCRIPTOR,
              ConfigAttributes :
                  (USB_CONFIG_ATTR_RESERVED | USB_CONFIG_ATTR_REMOTEWAKEUP),
              MaxPowerConsumption : USB_CONFIG_POWER_MA(500)},
    Interface1 : {Header : {Size : sizeof(XBOXConfigurationDescriptor.Interface1),
                            Type : DTYPE_Interface},
                  InterfaceNumber : INTERFACE_ID_Device,
                  AlternateSetting : 0x00,
                  TotalEndpoints : 2,
                  Class : 0xff,
                  SubClass : 0x5d,
                  Protocol : 0x01,
                  InterfaceStrIndex : NO_DESCRIPTOR},

    Interface4 : {Header : {Size : sizeof(XBOXConfigurationDescriptor.Interface4),
                            Type : DTYPE_Interface},
                  InterfaceNumber : INTERFACE_ID_XBOX_Security,
                  AlternateSetting : 0x00,
                  TotalEndpoints : 0,
                  Class : 0xff,
                  SubClass : 0xfd,
                  Protocol : 0x13,
                  InterfaceStrIndex : 4},
    InterfaceConfig : {
        Header : {Size : sizeof(USB_Descriptor_Interface_t), Type : DTYPE_Interface},
        InterfaceNumber : INTERFACE_ID_Config,
        AlternateSetting : 0,
        TotalEndpoints : 0,
        Class : 0xff,
        SubClass : 0xff,
        Protocol : 0xff,
        InterfaceStrIndex : NO_DESCRIPTOR,
    },
    InterfaceExtra : {
        Header : {Size : sizeof(USB_Descriptor_Interface_t), Type : DTYPE_Interface},
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
        Header : {Size : sizeof(XBOXConfigurationDescriptor.ReportINEndpoint11), Type : DTYPE_Endpoint},
        EndpointAddress : DEVICE_EPADDR_IN,
        Attributes : (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        EndpointSize : XINPUT_EPSIZE_IN,
        PollingIntervalMS : 1,
    },
    ReportOUTEndpoint12 : {
        Header : {Size : sizeof(XBOXConfigurationDescriptor.ReportOUTEndpoint12), Type : DTYPE_Endpoint},
        EndpointAddress : DEVICE_EPADDR_OUT,
        Attributes : (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        EndpointSize : XINPUT_EPSIZE_OUT,
        PollingIntervalMS : 1,
    },
};

USB_Descriptor_HID_Configuration_t HIDConfigurationDescriptor = {
    Config : {
        Header : {Size : sizeof(USB_Descriptor_Configuration_Header_t),
                  Type : DTYPE_Configuration},
        TotalConfigurationSize : sizeof(USB_Descriptor_HID_Configuration_t),
        TotalInterfaces : 4,
        ConfigurationNumber : 1,
        ConfigurationStrIndex : NO_DESCRIPTOR,
        .ConfigAttributes =
            (USB_CONFIG_ATTR_RESERVED | USB_CONFIG_ATTR_REMOTEWAKEUP),
        MaxPowerConsumption : USB_CONFIG_POWER_MA(500),
    },
    InterfaceHID : {
        Header :
            {Size : sizeof(USB_Descriptor_Interface_t), Type : DTYPE_Interface},
        InterfaceNumber : INTERFACE_ID_Device,
        AlternateSetting : 0,
        TotalEndpoints : 2,
        Class : HID_CSCP_HIDClass,
        SubClass : HID_CSCP_NonBootSubclass,
        Protocol : HID_CSCP_NonBootProtocol,
        InterfaceStrIndex : NO_DESCRIPTOR
    },
    EndpointInHID : {
        Header : {Size : sizeof(USB_Descriptor_Endpoint_t), Type : DTYPE_Endpoint},
        EndpointAddress : DEVICE_EPADDR_IN,
        Attributes :
            (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        EndpointSize : XINPUT_EPSIZE_IN,
        PollingIntervalMS : 1
    },
    EndpointOutHID : {
        Header : {Size : sizeof(USB_Descriptor_Endpoint_t), Type : DTYPE_Endpoint},
        EndpointAddress : DEVICE_EPADDR_OUT,
        Attributes :
            (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        EndpointSize : HID_EPSIZE_IN,
        PollingIntervalMS : 1
    },
    HIDDescriptor : {
        Header : {Size : sizeof(USB_HID_Descriptor_HID_t), Type : HID_DTYPE_HID},
        HIDSpec : VERSION_BCD(1, 0, 1),
        CountryCode : 0x00,
        TotalReportDescriptors : 1,
        HIDReportType : HID_DTYPE_Report,
        HIDReportLength : 0
    },
    InterfaceConfig : {
        Header : {Size : sizeof(USB_Descriptor_Interface_t), Type : DTYPE_Interface},
        InterfaceNumber : 1,
        AlternateSetting : 0,
        TotalEndpoints : 0,
        Class : 0xff,
        SubClass : 0xff,
        Protocol : 0xff,
        InterfaceStrIndex : NO_DESCRIPTOR,
    },
};

USB_Descriptor_MIDI_Configuration_t MIDIConfigurationDescriptor = {
    Config : {
        Header : {Size : sizeof(USB_Descriptor_Configuration_Header_t),
                  Type : DTYPE_Configuration},
        TotalConfigurationSize : sizeof(USB_Descriptor_MIDI_Configuration_t),
        TotalInterfaces : 4,
        ConfigurationNumber : 1,
        ConfigurationStrIndex : NO_DESCRIPTOR,
        .ConfigAttributes =
            (USB_CONFIG_ATTR_RESERVED | USB_CONFIG_ATTR_REMOTEWAKEUP),
        MaxPowerConsumption : USB_CONFIG_POWER_MA(500),
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
            (sizeof(USB_Descriptor_MIDI_Configuration_t) -
             offsetof(USB_Descriptor_MIDI_Configuration_t, Audio_StreamInterface_SPC))
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
            EndpointAddress : DEVICE_EPADDR_IN,
            Attributes : (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC |
                          ENDPOINT_USAGE_DATA),
            EndpointSize : HID_EPSIZE_IN,
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
            EndpointAddress : DEVICE_EPADDR_OUT,
            Attributes : (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC |
                          ENDPOINT_USAGE_DATA),
            EndpointSize : HID_EPSIZE_OUT,
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
        Header : {Size : sizeof(USB_Descriptor_Interface_t), Type : DTYPE_Interface},
        InterfaceNumber : 1,
        AlternateSetting : 0,
        TotalEndpoints : 0,
        Class : 0xff,
        SubClass : 0xff,
        Protocol : 0xff,
        InterfaceStrIndex : NO_DESCRIPTOR,
    },
};