#include "descriptors.h"
// We can't use WideStrings below, as the pico has four byte widestrings, and we need them to be two-byte.

/** Language descriptor structure. This descriptor, located in FLASH memory, is
 * returned when the host requests the string descriptor with index 0 (the first
 * index). It is actually an array of 16-bit integers, which indicate via the
 * language ID table available at USB.org what languages the device supports for
 * its string descriptors.
 */
const PROGMEM STRING_DESCRIPTOR languageString = USB_STRING_DESCRIPTOR_ARRAY(LANGUAGE_ID_ENG);
/** Manufacturer descriptor string. This is a Unicode string containing the
 * manufacturer's details in human readable form, and is read out upon request
 * by the host when the appropriate string ID is requested, listed in the Device
 *  Descriptor.
 */
const PROGMEM STRING_DESCRIPTOR manufacturerString = USB_STRING_DESCRIPTOR_ARRAY('s', 'a', 'n', 'j', 'a', 'y', '9', '0', '0');
/** Product descriptor string. This is a Unicode string containing the product's
 * details in human readable form, and is read out upon request by the host when
 * the appropriate string ID is requested, listed in the Device Descriptor.
 */
const PROGMEM STRING_DESCRIPTOR productString = USB_STRING_DESCRIPTOR_ARRAY('A', 'r', 'd', 'w', 'i', 'i', 'n', 'o');

/**
 * Descriptor used by the Xbox 360 to determine if a controller supports authentication
 */
const PROGMEM STRING_DESCRIPTOR xboxString = USB_STRING_DESCRIPTOR_ARRAY(
    'X', 'b', 'o', 'x', ' ', 'S', 'e', 'c', 'u', 'r', 'i', 't', 'y',
    ' ', 'M', 'e', 't', 'h', 'o', 'd', ' ', '3', ',', ' ',
    'V', 'e', 'r', 's', 'i', 'o', 'n', ' ', '1', '.', '0', '0', ',',
    ' ', 0xa9, ' ', '2', '0', '0', '5', ' ',
    'M', 'i', 'c', 'r', 'o', 's', 'o', 'f', 't', ' ',
    'C', 'o', 'r', 'p', 'o', 'r', 'a', 't', 'i', 'o', 'n', '.', ' ',
    'A', 'l', 'l', ' ', 'r', 'i', 'g', 'h', 't', 's', ' ',
    'r', 'e', 's', 'e', 'r', 'v', 'e', 'd', '.');
const PROGMEM STRING_DESCRIPTOR *const PROGMEM descriptorStrings[] = {
    &languageString, &manufacturerString, &productString};

/* A Microsoft-proprietary extension. String address 0xEE is used by
Windows for "OS Descriptors", which in this case allows us to indicate
that our device has a Compatible ID to provide. */
const PROGMEM OS_DESCRIPTOR OSDescriptorString = {
    bLength : sizeof(OS_DESCRIPTOR),
    bDescriptorType : USB_DESCRIPTOR_STRING,
    Signature : {'M', 'S', 'F', 'T', '1', '0', '0'},
    VendorCode : REQ_GET_OS_FEATURE_DESCRIPTOR,
    Reserved : 0
};

const PROGMEM USB_DEVICE_DESCRIPTOR deviceDescriptor = {
    bLength : sizeof(deviceDescriptor),
    bDescriptorType : USB_DESCRIPTOR_DEVICE,
    bcdUSB : VERSION_BCD(2, 0, 0),
    bDeviceClass : USB_CSCP_NoDeviceClass,
    bDeviceSubClass : USB_CSCP_NoDeviceSubclass,
    bDeviceProtocol : USB_CSCP_NoDeviceProtocol,
    bMaxPacketSize0 : ENDPOINT_SIZE,
    idVendor : ARDWIINO_VID,
    idProduct : ARDWIINO_PID,
    bcdDevice : VERSION_BCD(VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION),
    iManufacturer : 0x01,
    iProduct : 0x02,
    iSerialNumber : 0x03,
    bNumConfigurations : 1
};

const PROGMEM CONFIGURATION_XBOX_DESCRIPTOR XBOXConfigurationDescriptor = {
    Config : {
        bLength : sizeof(USB_CONFIGURATION_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_CONFIGURATION,
        wTotalLength : sizeof(CONFIGURATION_XBOX_DESCRIPTOR),
        bNumInterfaces : 4,
        bConfigurationValue : 1,
        iConfiguration : NO_DESCRIPTOR,
        bmAttributes :
            (USB_CONFIG_TATTR_RESERVED | USB_CONFIG_TATTR_REMOTEWAKEUP),
        bMaxPower : USB_CONFIG_POWER_MA(500)
    },
    Interface1 : {
        bLength : sizeof(USB_INTERFACE_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_INTERFACE,
        bInterfaceNumber : INTERFACE_ID_Device,
        bAlternateSetting : 0x00,
        bNumEndpoints : 2,
        bInterfaceClass : 0xFF,
        bInterfaceSubClass : 0x5D,
        bInterfaceProtocol : 0x01,
        iInterface : NO_DESCRIPTOR},
    Interface1ID : {
        bLength : sizeof(XBOX_ID_DESCRIPTOR),
        bDescriptorType : 0x21,
        reserved : {0x10, 0x01},
        subtype : 0x07,
        reserved2 : 0x25,
        bEndpointAddressIn : DEVICE_EPADDR_IN,
        bMaxDataSizeIn : 0x14,
        reserved3 : {0x03, 0x03, 0x03, 0x04, 0x13},
        bEndpointAddressOut : DEVICE_EPADDR_OUT,
        bMaxDataSizeOut : 0x08,
        reserved4 : {0x03, 0x03},
    },
    ReportINEndpoint11 : {
        bLength : sizeof(USB_ENDPOINT_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_ENDPOINT,
        bEndpointAddress : DEVICE_EPADDR_IN,
        bmAttributes : (USB_TRANSFER_TYPE_INTERRUPT | ENDPOINT_TATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        wMaxPacketSize : 0x20,
        bInterval : 1,
    },
    ReportOUTEndpoint12 : {
        bLength : sizeof(USB_ENDPOINT_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_ENDPOINT,
        bEndpointAddress : DEVICE_EPADDR_OUT,
        bmAttributes : (USB_TRANSFER_TYPE_INTERRUPT | ENDPOINT_TATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        wMaxPacketSize : 0x20,
        bInterval : 1,
    },

    Interface2 : {
        bLength : sizeof(USB_INTERFACE_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_INTERFACE,
        bInterfaceNumber : 1,
        bAlternateSetting : 0x00,
        bNumEndpoints : 4,
        bInterfaceClass : 0xFF,
        bInterfaceSubClass : 0x5D,
        bInterfaceProtocol : 0x03,
        iInterface : 0},
    UnkownDescriptor2 : {0x1B, 0x21, 0x00, 0x01, 0x01, 0x01, 0x83, 0x40, 0x01, 0x04,
                         0x20, 0x16, 0x85, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16,
                         0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    ReportINEndpoint21 : {
        bLength : sizeof(USB_ENDPOINT_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_ENDPOINT,
        bEndpointAddress : ENDPOINT_IN | 3,
        bmAttributes : (USB_TRANSFER_TYPE_INTERRUPT | ENDPOINT_TATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        wMaxPacketSize : 0x20,
        bInterval : 2,
    },
    ReportOUTEndpoint22 : {
        bLength : sizeof(USB_ENDPOINT_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_ENDPOINT,
        bEndpointAddress : ENDPOINT_OUT | 4,
        bmAttributes : (USB_TRANSFER_TYPE_INTERRUPT | ENDPOINT_TATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        wMaxPacketSize : 0x20,
        bInterval : 4,
    },
    ReportINEndpoint23 : {
        bLength : sizeof(USB_ENDPOINT_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_ENDPOINT,
        bEndpointAddress : ENDPOINT_IN | 5,
        bmAttributes : (USB_TRANSFER_TYPE_INTERRUPT | ENDPOINT_TATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        wMaxPacketSize : 0x20,
        bInterval : 0x40,
    },
    ReportOUTEndpoint24 : {
        bLength : sizeof(USB_ENDPOINT_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_ENDPOINT,
        bEndpointAddress : ENDPOINT_OUT | 5,
        bmAttributes : (USB_TRANSFER_TYPE_INTERRUPT | ENDPOINT_TATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        wMaxPacketSize : 0x20,
        bInterval : 0x10,
    },
    Interface3 : {
        bLength : sizeof(USB_INTERFACE_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_INTERFACE,
        bInterfaceNumber : 2,
        bAlternateSetting : 0x00,
        bNumEndpoints : 1,
        bInterfaceClass : 0xFF,
        bInterfaceSubClass : 0x5D,
        bInterfaceProtocol : 0x02,
        iInterface : 0},
    UnkownDescriptor3 : {
        0x09, 0x21, 0x00, 0x01, 0x01, 0x22, 0x86, 0x07, 0x00},
    ReportINEndpoint31 : {
        bLength : sizeof(XBOXConfigurationDescriptor.ReportINEndpoint31),
        bDescriptorType : USB_DESCRIPTOR_ENDPOINT,
        bEndpointAddress : ENDPOINT_IN | 6,
        bmAttributes : (USB_TRANSFER_TYPE_INTERRUPT | ENDPOINT_TATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        wMaxPacketSize : 0x20,
        bInterval : 16,
    },
    Interface4 : {
        bLength : sizeof(USB_INTERFACE_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_INTERFACE,
        bInterfaceNumber : INTERFACE_ID_XBOX_Security,
        bAlternateSetting : 0x00,
        bNumEndpoints : 0,
        bInterfaceClass : 0xFF,
        bInterfaceSubClass : 0xFD,
        bInterfaceProtocol : 0x13,
        iInterface : 4},
    UnkownDescriptor4 : {0x06, 0x41, 0x00, 0x01, 0x01, 0x03},
};

const PROGMEM HID_CONFIGURATION_DESCRIPTOR HIDConfigurationDescriptor = {
    Config : {
        bLength : sizeof(USB_CONFIGURATION_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_CONFIGURATION,
        wTotalLength : sizeof(HID_CONFIGURATION_DESCRIPTOR),
        bNumInterfaces : 4,
        bConfigurationValue : 1,
        iConfiguration : NO_DESCRIPTOR,
        bmAttributes :
            (USB_CONFIG_TATTR_RESERVED | USB_CONFIG_TATTR_REMOTEWAKEUP),
        bMaxPower : USB_CONFIG_POWER_MA(500),
    },
    InterfaceHID : {
        bLength : sizeof(USB_INTERFACE_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_INTERFACE,
        bInterfaceNumber : INTERFACE_ID_Device,
        bAlternateSetting : 0,
        bNumEndpoints : 2,
        bInterfaceClass : HID_CSCP_HIDClass,
        bInterfaceSubClass : HID_CSCP_NonBootSubclass,
        bInterfaceProtocol : HID_CSCP_NonBootProtocol,
        iInterface : NO_DESCRIPTOR
    },
    HIDDescriptor : {
        bLength : sizeof(USB_HID_DESCRIPTOR),
        bDescriptorType : HID_DESCRIPTOR_HID,
        bcdHID : VERSION_BCD(1, 0, 1),
        bCountryCode : 0x00,
        bNumDescriptors : 1,
        bDescrType : HID_DESCRIPTOR_REPORT,
        wDescriptorLength : 0
    },
    EndpointInHID : {
        bLength : sizeof(USB_ENDPOINT_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_ENDPOINT,
        bEndpointAddress : DEVICE_EPADDR_IN,
        bmAttributes :
            (USB_TRANSFER_TYPE_INTERRUPT | ENDPOINT_TATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        wMaxPacketSize : 0x20,
        bInterval : 1
    },
    EndpointOutHID : {
        bLength : sizeof(USB_ENDPOINT_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_ENDPOINT,
        bEndpointAddress : DEVICE_EPADDR_OUT,
        bmAttributes :
            (USB_TRANSFER_TYPE_INTERRUPT | ENDPOINT_TATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        wMaxPacketSize : 0x08,
        bInterval : 1
    },
    InterfaceConfig : {
        bLength : sizeof(USB_INTERFACE_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_INTERFACE,
        bInterfaceNumber : 1,
        bAlternateSetting : 0,
        bNumEndpoints : 0,
        bInterfaceClass : 0xff,
        bInterfaceSubClass : 0xff,
        bInterfaceProtocol : 0xff,
        iInterface : NO_DESCRIPTOR,
    },
    InterfaceExtra : {
        bLength : sizeof(USB_INTERFACE_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_INTERFACE,
        bInterfaceNumber : 2,
        bAlternateSetting : 0,
        bNumEndpoints : 0,
        bInterfaceClass : 0xff,
        bInterfaceSubClass : 0xff,
        bInterfaceProtocol : 0xff,
        iInterface : NO_DESCRIPTOR,
    },
    Interface4 : {
        bLength : sizeof(XBOXConfigurationDescriptor.Interface4),
        bDescriptorType : USB_DESCRIPTOR_INTERFACE,
        bInterfaceNumber : INTERFACE_ID_XBOX_Security,
        bAlternateSetting : 0x00,
        bNumEndpoints : 0,
        bInterfaceClass : 0xFF,
        bInterfaceSubClass : 0xFD,
        bInterfaceProtocol : 0x13,
        iInterface : 4},
    UnkownDescriptor4 : {0x06, 0x41, 0x00, 0x01, 0x01, 0x03},
};

const PROGMEM MIDI_CONFIGURATION_DESCRIPTOR MIDIConfigurationDescriptor = {
    Config : {
        bLength : sizeof(USB_CONFIGURATION_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_CONFIGURATION,
        wTotalLength : sizeof(MIDI_CONFIGURATION_DESCRIPTOR),
        bNumInterfaces : 3,
        bConfigurationValue : 1,
        iConfiguration : NO_DESCRIPTOR,
        .bmAttributes =
            (USB_CONFIG_TATTR_RESERVED | USB_CONFIG_TATTR_REMOTEWAKEUP),
        bMaxPower : USB_CONFIG_POWER_MA(500),
    },
    Interface_AudioControl : {
        bLength : sizeof(USB_INTERFACE_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_INTERFACE,
        bInterfaceNumber : INTERFACE_ID_ControlStream,
        bAlternateSetting : 0x00,
        bNumEndpoints : 0,
        bInterfaceClass : AUDIO_CSCP_AudioClass,
        bInterfaceSubClass : AUDIO_CSCP_ControlSubclass,
        bInterfaceProtocol : AUDIO_CSCP_ControlProtocol,
        iInterface : NO_DESCRIPTOR
    },
    Audio_ControlInterface_SPC : {
        bLength : sizeof(AUDIO_INTERFACE_AC_DESCRIPTOR),
        bDescriptorType : AUDIO_DESCRIPTOR_CSInterface,
        bDescriptorSubtype : AUDIO_DSUBTYPE_CSInterface_Header,
        bcdADC : VERSION_BCD(1, 0, 0),
        wTotalLength : sizeof(AUDIO_INTERFACE_AC_DESCRIPTOR),
        bInCollection : 1,
        bInterfaceNumbers : INTERFACE_ID_AudioStream,
    },
    Interface_AudioStream : {
        bLength : sizeof(USB_INTERFACE_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_INTERFACE,
        bInterfaceNumber : INTERFACE_ID_AudioStream,
        bAlternateSetting : 0x00,
        bNumEndpoints : 2,
        bInterfaceClass : AUDIO_CSCP_AudioClass,
        bInterfaceSubClass : AUDIO_CSCP_MIDIStreamingSubclass,
        bInterfaceProtocol : AUDIO_CSCP_StreamingProtocol,
        iInterface : NO_DESCRIPTOR
    },
    Audio_StreamInterface_SPC : {
        bLength : sizeof(MIDI_AUDIOINTERFACE_AS_DESCRIPTOR),
        bDescriptorType : AUDIO_DESCRIPTOR_CSInterface,
        bDescriptorSubtype : AUDIO_DSUBTYPE_CSInterface_General,
        bcdMSC : VERSION_BCD(1, 0, 0),
        wTotalLength :
            (sizeof(MIDI_CONFIGURATION_DESCRIPTOR) -
             offsetof(MIDI_CONFIGURATION_DESCRIPTOR, Audio_StreamInterface_SPC))
    },

    MIDI_In_Jack_Emb : {
        bLength : sizeof(MIDI_INPUTJACK_DESCRIPTOR),
        bDescriptorType : AUDIO_DESCRIPTOR_CSInterface,
        bDescriptorSubtype : AUDIO_DSUBTYPE_CSInterface_InputTerminal,
        bJackType : MIDI_JACKTYPE_Embedded,
        bJackID : 0x01,
        iJack : NO_DESCRIPTOR
    },
    MIDI_In_Jack_Ext : {
        bLength : sizeof(MIDI_INPUTJACK_DESCRIPTOR),
        bDescriptorType : AUDIO_DESCRIPTOR_CSInterface,
        bDescriptorSubtype : AUDIO_DSUBTYPE_CSInterface_InputTerminal,
        bJackType : MIDI_JACKTYPE_External,
        bJackID : 0x02,
        iJack : NO_DESCRIPTOR
    },
    MIDI_Out_Jack_Emb : {
        bLength : sizeof(MIDI_OUTPUTJACK_DESCRIPTOR),
        bDescriptorType : AUDIO_DESCRIPTOR_CSInterface,
        bDescriptorSubtype : AUDIO_DSUBTYPE_CSInterface_OutputTerminal,
        bJackType : MIDI_JACKTYPE_Embedded,
        bJackID : 0x03,
        bNrInputPins : 1,
        baSourceID : {0x02},
        baSourcePin : {0x01},
        iJack : NO_DESCRIPTOR
    },
    MIDI_Out_Jack_Ext : {
        bLength : sizeof(MIDI_OUTPUTJACK_DESCRIPTOR),
        bDescriptorType : AUDIO_DESCRIPTOR_CSInterface,
        bDescriptorSubtype : AUDIO_DSUBTYPE_CSInterface_OutputTerminal,
        bJackType : MIDI_JACKTYPE_External,
        bJackID : 0x04,
        bNrInputPins : 1,
        baSourceID : {0x01},
        baSourcePin : {0x01},
        iJack : NO_DESCRIPTOR
    },
    MIDI_In_Jack_Endpoint : {
        bLength : sizeof(AUDIO_STREAMENDPOINT_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_ENDPOINT,
        bEndpointAddress : DEVICE_EPADDR_OUT,
        bmAttributes : (USB_TRANSFER_TYPE_BULK | ENDPOINT_TATTR_NO_SYNC |
                        ENDPOINT_USAGE_DATA),
        wMaxPacketSize : 0x08,
        bInterval : 1,
        bRefresh : 0,
        bSynchAddress : 0
    },
    MIDI_In_Jack_Endpoint_SPC : {
        bLength : sizeof(MIDI_JACK_ENDPOINT_DESCRIPTOR),
        bDescriptorType : AUDIO_DTYPE_CSEndpoint,
        bDescriptorSubtype : AUDIO_DSUBTYPE_CSEndpoint_General,
        bNumEmbMIDIJack : 0x01,
        bAssocJackID : {0x01}
    },
    MIDI_Out_Jack_Endpoint : {
        bLength : sizeof(AUDIO_STREAMENDPOINT_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_ENDPOINT,
        bEndpointAddress : DEVICE_EPADDR_IN,
        bmAttributes : (USB_TRANSFER_TYPE_BULK | ENDPOINT_TATTR_NO_SYNC |
                        ENDPOINT_USAGE_DATA),
        wMaxPacketSize : 0x20,
        bInterval : 1,
        bRefresh : 0,
        bSynchAddress : 0
    },
    MIDI_Out_Jack_Endpoint_SPC : {
        bLength : sizeof(MIDI_JACK_ENDPOINT_DESCRIPTOR),
        bDescriptorType : AUDIO_DTYPE_CSEndpoint,
        bDescriptorSubtype : AUDIO_DSUBTYPE_CSEndpoint_General,
        bNumEmbMIDIJack : 0x01,
        bAssocJackID : {0x03}
    },
    InterfaceConfig : {
        bLength : sizeof(USB_INTERFACE_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_INTERFACE,
        bInterfaceNumber : 1,
        bAlternateSetting : 0,
        bNumEndpoints : 0,
        bInterfaceClass : 0xff,
        bInterfaceSubClass : 0xff,
        bInterfaceProtocol : 0xff,
        iInterface : NO_DESCRIPTOR,
    },
};