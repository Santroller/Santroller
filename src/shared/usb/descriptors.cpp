#include "descriptors.h"

#include "Usb.h"
#include "commands.h"
#include "config.h"
#include "controllers.h"
#include "keyboard_mouse.h"
#include "ps3_wii_switch.h"
#include "usbhid.h"
#include "util.h"
// We can't use WideStrings below, as the pico has four byte widestrings, and we need them to be two-byte.


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
    bcdUSB : USB_VERSION_BCD(2, 0, 0),
    bDeviceClass : USB_CLASS_USE_CLASS_INFO,
    bDeviceSubClass : USB_CLASS_USE_CLASS_INFO,
    bDeviceProtocol : USB_CLASS_USE_CLASS_INFO,
    bMaxPacketSize0 : ENDPOINT_SIZE,
    idVendor : ARDWIINO_VID,
    idProduct : ARDWIINO_PID,
    bcdDevice : USB_VERSION_BCD(VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION),
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
            (USB_CONFIG_ATTRIBUTE_RESERVED | USB_CONFIG_ATTRIBUTE_REMOTEWAKEUP),
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
        iInterface : NO_DESCRIPTOR
    },
    Interface1ID : {
        bLength : sizeof(XBOX_ID_DESCRIPTOR),
        bDescriptorType : 0x21,
        reserved : {0x10, 0x01},
        subtype : SUB_TYPE,
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
        iInterface : 0
    },
    UnkownDescriptor2 : {0x1B, 0x21, 0x00, 0x01, 0x01, 0x01, XINPUT_EXTRA_1, 0x40, 0x01, XINPUT_EXTRA_2,
                         0x20, 0x16, XINPUT_EXTRA_3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16,
                         XINPUT_EXTRA_4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    ReportINEndpoint21 : {
        bLength : sizeof(USB_ENDPOINT_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_ENDPOINT,
        bEndpointAddress : XINPUT_EXTRA_1,
        bmAttributes : (USB_TRANSFER_TYPE_INTERRUPT | ENDPOINT_TATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        wMaxPacketSize : 0x20,
        bInterval : 2,
    },
    ReportOUTEndpoint22 : {
        bLength : sizeof(USB_ENDPOINT_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_ENDPOINT,
        bEndpointAddress : XINPUT_EXTRA_2,
        bmAttributes : (USB_TRANSFER_TYPE_INTERRUPT | ENDPOINT_TATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        wMaxPacketSize : 0x20,
        bInterval : 4,
    },
    ReportINEndpoint23 : {
        bLength : sizeof(USB_ENDPOINT_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_ENDPOINT,
        bEndpointAddress : XINPUT_EXTRA_3,
        bmAttributes : (USB_TRANSFER_TYPE_INTERRUPT | ENDPOINT_TATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        wMaxPacketSize : 0x20,
        bInterval : 0x40,
    },
    ReportOUTEndpoint24 : {
        bLength : sizeof(USB_ENDPOINT_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_ENDPOINT,
        bEndpointAddress : XINPUT_EXTRA_4,
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
        iInterface : 0
    },
    UnkownDescriptor3 : {
        0x09, 0x21, 0x00, 0x01, 0x01, 0x22, XINPUT_EXTRA_5, 0x07, 0x00},
    ReportINEndpoint31 : {
        bLength : sizeof(XBOXConfigurationDescriptor.ReportINEndpoint31),
        bDescriptorType : USB_DESCRIPTOR_ENDPOINT,
        bEndpointAddress : XINPUT_EXTRA_5,
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
        iInterface : 4
    },
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
            (USB_CONFIG_ATTRIBUTE_RESERVED | USB_CONFIG_ATTRIBUTE_REMOTEWAKEUP),
        bMaxPower : USB_CONFIG_POWER_MA(500),
    },
    InterfaceHID : {
        bLength : sizeof(USB_INTERFACE_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_INTERFACE,
        bInterfaceNumber : INTERFACE_ID_Device,
        bAlternateSetting : 0,
        bNumEndpoints : 2,
        bInterfaceClass : HID_INTF,
        bInterfaceSubClass : USB_HID_PROTOCOL_NONE,
        bInterfaceProtocol : USB_HID_PROTOCOL_NONE,
        iInterface : NO_DESCRIPTOR
    },
    HIDDescriptor : {
        bLength : sizeof(USB_HID_DESCRIPTOR),
        bDescriptorType : HID_DESCRIPTOR_HID,
        bcdHID : USB_VERSION_BCD(1, 0, 1),
        bCountryCode : 0x00,
        bNumDescriptors : 1,
        bDescrType : HID_DESCRIPTOR_REPORT,
#if SUPPORTS_KEYBOARD
        wDescriptorLength : sizeof(keyboard_mouse_descriptor)
#else
        wDescriptorLength : sizeof(ps3_descriptor)
#endif
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
        iInterface : 4
    },
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
            (USB_CONFIG_ATTRIBUTE_RESERVED | USB_CONFIG_ATTRIBUTE_REMOTEWAKEUP),
        bMaxPower : USB_CONFIG_POWER_MA(500),
    },
    Interface_AudioControl : {
        bLength : sizeof(USB_INTERFACE_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_INTERFACE,
        bInterfaceNumber : INTERFACE_ID_ControlStream,
        bAlternateSetting : 0x00,
        bNumEndpoints : 0,
        bInterfaceClass : USB_CLASS_AUDIO,
        bInterfaceSubClass : USB_SUBCLASS_CONTROL,
        bInterfaceProtocol : USB_PROTOCOL_CONTROL,
        iInterface : NO_DESCRIPTOR
    },
    Audio_ControlInterface_SPC : {
        bLength : sizeof(AUDIO_INTERFACE_AC_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_AUDIO_CSINTERFACE,
        bDescriptorSubtype : USB_AUDIO_SUBTYPE_CSINTERFACE_HEADER,
        bcdADC : USB_VERSION_BCD(1, 0, 0),
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
        bInterfaceClass : USB_CLASS_AUDIO,
        bInterfaceSubClass : USB_SUBCLASS_MIDI_STREAMING,
        bInterfaceProtocol : USB_PROTOCOL_STREAMING,
        iInterface : NO_DESCRIPTOR
    },
    Audio_StreamInterface_SPC : {
        bLength : sizeof(MIDI_AUDIOINTERFACE_AS_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_AUDIO_CSINTERFACE,
        bDescriptorSubtype : USB_AUDIO_SUBTYPE_CSINTERFACE_GENERAL,
        bcdMSC : USB_VERSION_BCD(1, 0, 0),
        wTotalLength :
            (sizeof(MIDI_CONFIGURATION_DESCRIPTOR) -
             offsetof(MIDI_CONFIGURATION_DESCRIPTOR, Audio_StreamInterface_SPC))
    },

    MIDI_In_Jack_Emb : {
        bLength : sizeof(MIDI_INPUTJACK_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_AUDIO_CSINTERFACE,
        bDescriptorSubtype : USB_AUDIO_SUBTYPE_CSINTERFACE_INPUT_TERMINAL,
        bJackType : MIDI_JACK_TYPE_EMBEDDED,
        bJackID : 0x01,
        iJack : NO_DESCRIPTOR
    },
    MIDI_In_Jack_Ext : {
        bLength : sizeof(MIDI_INPUTJACK_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_AUDIO_CSINTERFACE,
        bDescriptorSubtype : USB_AUDIO_SUBTYPE_CSINTERFACE_INPUT_TERMINAL,
        bJackType : MIDI_JACK_TYPE_EXTERNAL,
        bJackID : 0x02,
        iJack : NO_DESCRIPTOR
    },
    MIDI_Out_Jack_Emb : {
        bLength : sizeof(MIDI_OUTPUTJACK_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_AUDIO_CSINTERFACE,
        bDescriptorSubtype : USB_AUDIO_SUBTYPE_CSINTERFACE_OUTPUT_TERMINAL,
        bJackType : MIDI_JACK_TYPE_EMBEDDED,
        bJackID : 0x03,
        bNrInputPins : 1,
        baSourceID : {0x02},
        baSourcePin : {0x01},
        iJack : NO_DESCRIPTOR
    },
    MIDI_Out_Jack_Ext : {
        bLength : sizeof(MIDI_OUTPUTJACK_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_AUDIO_CSINTERFACE,
        bDescriptorSubtype : USB_AUDIO_SUBTYPE_CSINTERFACE_OUTPUT_TERMINAL,
        bJackType : MIDI_JACK_TYPE_EXTERNAL,
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
        bDescriptorType : USB_DESCRIPTOR_AUDIO_CSENDPOINT,
        bDescriptorSubtype : USB_AUDIO_SUBTYPE_CSENDPOINT_GENERAL,
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
        bDescriptorType : USB_DESCRIPTOR_AUDIO_CSENDPOINT,
        bDescriptorSubtype : USB_AUDIO_SUBTYPE_CSENDPOINT_GENERAL,
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

const PROGMEM uint8_t ps3_init[] = {0x21, 0x26, 0x01, 0x07,
                                    0x00, 0x00, 0x00, 0x00};
const PROGMEM char board[] = BOARD;
uint8_t idle_rate;
uint8_t protocol_mode = HID_RPT_PROTOCOL;
bool controlRequestValid(const uint8_t requestType, const uint8_t request, const uint16_t wValue, const uint16_t wIndex, const uint16_t wLength) {
    if (requestType == (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS)) {
        if (request == COMMAND_REBOOT) {
            return true;
        }
        if (request == COMMAND_JUMP_BOOTLOADER) {
            return true;
        }
    } else if (requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS) && (request == COMMAND_READ_CONFIG || request == COMMAND_READ_BOARD || request == COMMAND_READ_F_CPU)) {
        return true;
    } else if (requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_VENDOR)) {
        return true;
    } else if (requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_VENDOR)) {
        return true;
    } else if (requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS) && wIndex == 0x0300) {
        return true;
    } else if (consoleType == PC && request == USB_REQUEST_CLEAR_FEATURE && (wIndex == DEVICE_EPADDR_IN || wIndex == DEVICE_EPADDR_OUT)) {
        return true;
    } else if (request == HID_REQUEST_GET_PROTOCOL && requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS)) {
        return true;
    } else if (request == HID_REQUEST_SET_PROTOCOL && requestType == (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS)) {
        return true;
    } else if (request == HID_REQUEST_GET_IDLE && requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS)) {
        return true;
    } else if (request == HID_REQUEST_SET_IDLE && requestType == (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS)) {
        return true;
    } else if (request == HID_REQUEST_SET_REPORT && requestType == (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS)) {
        return true;
    } else if (request == HID_REQUEST_GET_REPORT && requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS)) {
        return true;
    }
    return false;
}

uint16_t controlRequest(const uint8_t requestType, const uint8_t request, const uint16_t wValue, const uint16_t wIndex, const uint16_t wLength, void *requestBuffer) {
    if (requestType == (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS)) {
        if (request == COMMAND_REBOOT) {
            reboot();
        }
        if (request == COMMAND_JUMP_BOOTLOADER) {
            bootloader();
        }
        if (request == COMMAND_READ_CONFIG) {
            memcpy_P(requestBuffer, config, sizeof(config));
            return sizeof(config);
        }
    } else if (requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS) && request == COMMAND_READ_CONFIG) {
        memcpy_P(requestBuffer, config, sizeof(config));
        return sizeof(config);
    } else if (requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS) && request == COMMAND_READ_BOARD) {
        memcpy(requestBuffer, board, sizeof(board));
        return sizeof(board);
    } else if (requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS) && request == COMMAND_READ_F_CPU) {
        char f_cpu[] = STR(F_CPU);
        memcpy(requestBuffer, f_cpu, sizeof(f_cpu));
        return sizeof(f_cpu);
    } else if (requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_VENDOR)) {
        if (request == HID_REQUEST_GET_REPORT && wIndex == INTERFACE_ID_Device && wValue == 0x0000) {
            memcpy_P(requestBuffer, capabilities1, sizeof(capabilities1));
            return sizeof(capabilities1);
        } else if (request == REQ_GET_OS_FEATURE_DESCRIPTOR && wIndex == DESC_EXTENDED_PROPERTIES_DESCRIPTOR && wValue == INTERFACE_ID_Config) {
            memcpy_P(requestBuffer, &ExtendedIDs, ExtendedIDs.TotalLength);
            return ExtendedIDs.TotalLength;
        } else if (request == HID_REQUEST_GET_REPORT && wIndex == INTERFACE_ID_Device && wValue == 0x0100) {
            memcpy_P(requestBuffer, capabilities2, sizeof(capabilities2));
            return sizeof(capabilities2);
        }
    } else if (requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_VENDOR)) {
        if (request == REQ_GET_OS_FEATURE_DESCRIPTOR && wIndex == DESC_EXTENDED_COMPATIBLE_ID_DESCRIPTOR) {
            memcpy_P(requestBuffer, &DevCompatIDs, DevCompatIDs.TotalLength);
            if (consoleType == PC_XINPUT) {
                OS_COMPATIBLE_ID_DESCRIPTOR *compat = (OS_COMPATIBLE_ID_DESCRIPTOR *)requestBuffer;
                compat->TotalSections = 2;
            }
            return DevCompatIDs.TotalLength;
        } else if (request == HID_REQUEST_GET_REPORT && wIndex == 0x00 && wValue == 0x0000) {
            memcpy_P(requestBuffer, XBOX_ID, sizeof(XBOX_ID));
            return sizeof(XBOX_ID);
        }
    } else if (requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS) && wIndex == 0x0300) {
        if (consoleType == PS3) {
            memcpy_P(requestBuffer, ps3_init, sizeof(ps3_init));
            if (DEVICE_TYPE <= ROCK_BAND_DRUMS) {
                ((uint8_t *)requestBuffer)[3] = 0x00;
            } else if (DEVICE_TYPE <= GUITAR_HERO_DRUMS) {
                ((uint8_t *)requestBuffer)[3] = 0x06;
            }
            return sizeof(ps3_init);
        } else if (consoleType == PC) {
            consoleType = PS3;
            reset_usb();
            return 0;
        }
    } else if (request == HID_REQUEST_GET_PROTOCOL && requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS)) {
        ((uint8_t *)requestBuffer)[0] = protocol_mode;
        return 1;
    } else if (request == HID_REQUEST_SET_PROTOCOL && requestType == (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS)) {
        protocol_mode = (uint8_t)wValue;
        return 0;
    } else if (request == HID_REQUEST_GET_IDLE && requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS)) {
        ((uint8_t *)requestBuffer)[0] = idle_rate;
        return 1;
    } else if (request == HID_REQUEST_SET_IDLE && requestType == (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS)) {
        idle_rate = (wValue >> 8) & 0xff;
        return 0;
    } else if (request == HID_REQUEST_SET_REPORT && requestType == (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS)) {
        return 0;
    } else if (request == HID_REQUEST_GET_REPORT && requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS)) {
        return 0;
    } else if (consoleType == PC && request == USB_REQUEST_CLEAR_FEATURE && (wIndex == DEVICE_EPADDR_IN || wIndex == DEVICE_EPADDR_OUT)) {
        consoleType = SWITCH;
        reset_usb();
        return 0;
    }
    return 0;
}
const uint16_t vid[] = {0x0F0D, 0x12ba, 0x12ba, 0x12ba,
                        0x12ba, ARDWIINO_VID, 0x1bad, 0x1bad};
const uint16_t pid[] = {0x0092, 0x0100, 0x0120, 0x0200,
                        0x0210, ARDWIINO_PID, 0x0004, 0x074B};
uint16_t descriptorRequest(const uint16_t wValue,
                           const uint16_t wIndex,
                           void *descriptorBuffer) {
    const uint8_t descriptorType = (wValue >> 8);
    const uint8_t descriptorNumber = (wValue & 0xFF);
    uint16_t size = NO_DESCRIPTOR;
    switch (descriptorType) {
        case USB_DESCRIPTOR_DEVICE: {
            size = sizeof(USB_DEVICE_DESCRIPTOR);
            memcpy_P(descriptorBuffer, &deviceDescriptor, size);
            USB_DEVICE_DESCRIPTOR *dev = (USB_DEVICE_DESCRIPTOR *)descriptorBuffer;
            if (consoleType == PS3 || consoleType == PC) {
                if (DEVICE_TYPE > GUITAR_HERO_GUITAR) {
                    dev->idVendor = SONY_VID;
                    switch (DEVICE_TYPE) {
                        case GUITAR_HERO_DRUMS:
                            dev->idProduct = PS3_GH_DRUM_PID;
                            break;
                        case GUITAR_HERO_GUITAR:
                            dev->idProduct = PS3_GH_GUITAR_PID;
                            break;
                        case ROCK_BAND_GUITAR:
                            dev->idProduct = PS3_RB_GUITAR_PID;
                            break;
                        case ROCK_BAND_DRUMS:
                            dev->idProduct = PS3_RB_DRUM_PID;
                            break;
                        case GUITAR_HERO_LIVE_GUITAR:
                            dev->idProduct = PS3WIIU_GHLIVE_DONGLE_PID;
                            break;
                        case DJ_HERO_TURNTABLE:
                            dev->idProduct = PS3_DJ_TURNTABLE_PID;
                            break;
                        default:
                            break;
                    }
                }
            } else if (consoleType == SWITCH) {
                dev->idVendor = HORI_VID;
                dev->idProduct = HORI_POKKEN_TOURNAMENT_DX_PRO_PAD_PID;
            } else if (consoleType == WII_RB) {
                dev->idVendor = WII_RB_VID;
                if (DEVICE_TYPE_IS_DRUM) {
                    dev->idProduct = WII_RB_DRUM_PID;
                } else {
                    dev->idProduct = WII_RB_GUITAR_PID;
                }
            }
            break;
        }
        case USB_DESCRIPTOR_CONFIGURATION: {
            if (consoleType == XBOX360 || consoleType == PC_XINPUT) {
                size = sizeof(CONFIGURATION_XBOX_DESCRIPTOR);
                memcpy_P(descriptorBuffer, &XBOXConfigurationDescriptor, size);
            } else if (consoleType == MIDI) {
                size = sizeof(MIDI_CONFIGURATION_DESCRIPTOR);
                memcpy_P(descriptorBuffer, &MIDIConfigurationDescriptor, size);
            } else {
                size = sizeof(HID_CONFIGURATION_DESCRIPTOR);
                memcpy_P(descriptorBuffer, &HIDConfigurationDescriptor, size);
                HID_CONFIGURATION_DESCRIPTOR *desc = (HID_CONFIGURATION_DESCRIPTOR *)descriptorBuffer;
                // TODO: test wii RB without this?
                // TODO: one cool thing is we don't actually need to support configuring when using 
                // Switch / PS3 modes, so we could just straight up define a configuration descriptor with no extra info
                // Such as our SWITCH_CONFIGURATION_DESCRIPTOR?
                // Or we just do this for all consoles if we don't want to hardcode duplicate  data
                if (consoleType == WII_RB) {
                    desc->Config.bNumInterfaces = 1;
                    desc->Config.wTotalLength = offsetof(HID_CONFIGURATION_DESCRIPTOR, InterfaceConfig);
                }
            }
            break;
        }
        case HID_DESCRIPTOR_REPORT: {
            read_hid_report_descriptor = true;
            const void *address;
            if (consoleType == KEYBOARD_MOUSE) {
                address = keyboard_mouse_descriptor;
                size = sizeof(keyboard_mouse_descriptor);
            } else if (consoleType == PS3 || consoleType == WII_RB || consoleType == SWITCH || consoleType == PC) {
                address = ps3_descriptor;
                size = sizeof(ps3_descriptor);
            }
            memcpy_P(descriptorBuffer, address, size);
            break;
        }
        case USB_DESCRIPTOR_STRING: {
            const uint8_t *str;
            if (descriptorNumber == 4) {
                str = (uint8_t *)&xboxString;
            } else if (descriptorNumber < 4) {
                // TODO: make this not require specific AVR stuff
#ifdef __AVR__
                str = (uint8_t *)pgm_read_word(descriptorStrings + descriptorNumber);
#else
                str = (uint8_t *)descriptorStrings[descriptorNumber];
#endif
            } else if (descriptorNumber == 0xEE) {
                str = (uint8_t *)&OSDescriptorString;
            } else {
                break;
            }
            size = pgm_read_byte(str + offsetof(STRING_DESCRIPTOR, bLength));
            memcpy_P(descriptorBuffer, str, size);
            break;
        }
    }
    return size;
}