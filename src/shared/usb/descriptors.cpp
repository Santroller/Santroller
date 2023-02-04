#include "descriptors.h"

#include "Usb.h"
#include "commands.h"
#include "config.h"
#include "controllers.h"
#include "io.h"
#include "keyboard_mouse.h"
#include "pins.h"
#include "ps3_wii_switch.h"
#include "usbhid.h"
#include "util.h"
#include "xsm3/xsm3.h"

#ifdef KV_KEY_1
const PROGMEM uint8_t kv_key_1[16] = KV_KEY_1;
const PROGMEM uint8_t kv_key_2[16] = KV_KEY_2;
#endif
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

const PROGMEM XBOX_360_CONFIGURATION_DESCRIPTOR XBOX360ConfigurationDescriptor = {
    Config : {
        bLength : sizeof(USB_CONFIGURATION_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_CONFIGURATION,
        wTotalLength : sizeof(XBOX_360_CONFIGURATION_DESCRIPTOR),
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
        reserved : {XINPUT_FLAGS, 0x01},
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
        bInterfaceNumber : INTERFACE_ID_Config,
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
        bInterfaceNumber : INTERFACE_ID_Padding,
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
        bLength : sizeof(XBOX360ConfigurationDescriptor.ReportINEndpoint31),
        bDescriptorType : USB_DESCRIPTOR_ENDPOINT,
        bEndpointAddress : XINPUT_EXTRA_5,
        bmAttributes : (USB_TRANSFER_TYPE_INTERRUPT | ENDPOINT_TATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        wMaxPacketSize : 0x20,
        bInterval : 16,
    },
    InterfaceSecurity : {
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

const PROGMEM UNIVERSAL_CONFIGURATION_DESCRIPTOR UniversalConfigurationDescriptor = {
    Config : {
        bLength : sizeof(USB_CONFIGURATION_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_CONFIGURATION,
        wTotalLength : sizeof(UNIVERSAL_CONFIGURATION_DESCRIPTOR),
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
        bInterfaceNumber : INTERFACE_ID_Config,
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
        bInterfaceNumber : INTERFACE_ID_Padding,
        bAlternateSetting : 0,
        bNumEndpoints : 0,
        bInterfaceClass : 0xff,
        bInterfaceSubClass : 0xff,
        bInterfaceProtocol : 0xff,
        iInterface : NO_DESCRIPTOR,
    },
    InterfaceSecurity : {
        bLength : sizeof(XBOX360ConfigurationDescriptor.InterfaceSecurity),
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

const PROGMEM XBOX_ONE_CONFIGURATION_DESCRIPTOR XBOXOneConfigurationDescriptor = {
    Config : {
        bLength : sizeof(USB_CONFIGURATION_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_CONFIGURATION,
        wTotalLength : sizeof(XBOX_ONE_CONFIGURATION_DESCRIPTOR),
        bNumInterfaces : 1,
        bConfigurationValue : 1,
        iConfiguration : NO_DESCRIPTOR,
        bmAttributes :
            (USB_CONFIG_ATTRIBUTE_RESERVED | USB_CONFIG_ATTRIBUTE_REMOTEWAKEUP),
        bMaxPower : USB_CONFIG_POWER_MA(500),
    },
    Interface : {
        bLength : sizeof(USB_INTERFACE_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_INTERFACE,
        bInterfaceNumber : INTERFACE_ID_Device,
        bAlternateSetting : 0,
        bNumEndpoints : 2,
        bInterfaceClass : 0xff,
        bInterfaceSubClass : 0x47,
        bInterfaceProtocol : 0xd0,
        iInterface : NO_DESCRIPTOR
    },
    EndpointIn : {
        bLength : sizeof(USB_ENDPOINT_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_ENDPOINT,
        bEndpointAddress : DEVICE_EPADDR_IN,
        bmAttributes :
            (USB_TRANSFER_TYPE_INTERRUPT | ENDPOINT_TATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        wMaxPacketSize : 0x40,
        bInterval : 1
    },
    EndpointOut : {
        bLength : sizeof(USB_ENDPOINT_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_ENDPOINT,
        bEndpointAddress : DEVICE_EPADDR_OUT,
        bmAttributes :
            (USB_TRANSFER_TYPE_INTERRUPT | ENDPOINT_TATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        wMaxPacketSize : 0x40,
        bInterval : 1
    }
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

const PROGMEM uint8_t ps3_init[] = {0x21, 0x26, 0x01, PS3_ID,
                                    0x00, 0x00, 0x00, 0x00};
const PROGMEM char board[] = ARDWIINO_BOARD;
const PROGMEM char f_cpu_descriptor_str[] = STR(F_CPU_FREQ);
uint8_t idle_rate;
uint8_t protocol_mode = HID_RPT_PROTOCOL;
bool controlRequestValid(const uint8_t requestType, const uint8_t request, const uint16_t wValue, const uint16_t wIndex, const uint16_t wLength) {
#ifdef KV_KEY_1
    switch (request) {
        case 0x81:
        case 0x82:
        case 0x87:
        case 0x83:
        case 0x84:
        case 0x86:
            return true;
    }
#endif
    if (consoleType != XBOX360 && requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_VENDOR) && request == 0x81) {
        return true;
    }
    if (requestType == (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS)) {
        switch (request) {
            case COMMAND_REBOOT:
            case COMMAND_JUMP_BOOTLOADER:
            case COMMAND_SET_DETECT:
            case HID_REQUEST_SET_PROTOCOL:
            case HID_REQUEST_SET_IDLE:
            case HID_REQUEST_SET_REPORT:
                return true;
        }
    }
    if (requestType == (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS) && request == USB_REQUEST_GET_INTERFACE) {
        return true;
    }
    if (requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS) && request == USB_REQUEST_SET_INTERFACE) {
        return true;
    }
    if (requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS)) {
        switch (request) {
            case COMMAND_READ_CONFIG:
            case COMMAND_READ_F_CPU:
            case COMMAND_READ_BOARD:
            case COMMAND_READ_DIGITAL:
            case COMMAND_READ_ANALOG:
            case COMMAND_READ_PS2:
            case COMMAND_READ_WII:
            case COMMAND_READ_DJ_LEFT:
            case COMMAND_READ_DJ_RIGHT:
            case COMMAND_READ_GH5:
            case COMMAND_READ_GHWT:
            case COMMAND_GET_EXTENSION_WII:
            case COMMAND_GET_EXTENSION_PS2:
            case HID_REQUEST_GET_PROTOCOL:
            case HID_REQUEST_GET_IDLE:
            case HID_REQUEST_GET_REPORT:
                return true;
        }
    } else if (requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_VENDOR)) {
        if (request == HID_REQUEST_GET_REPORT && wIndex == INTERFACE_ID_Device && wValue == 0x0000) {
            return true;
        } else if (request == REQ_GET_OS_FEATURE_DESCRIPTOR && wIndex == DESC_EXTENDED_PROPERTIES_DESCRIPTOR && wValue == INTERFACE_ID_Config) {
            return true;
        } else if (request == HID_REQUEST_GET_REPORT && wIndex == INTERFACE_ID_Device && wValue == 0x0100) {
            return true;
        }
    } else if (requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_DEVICE | USB_SETUP_TYPE_VENDOR) && request == REQ_GET_OS_FEATURE_DESCRIPTOR && wIndex == DESC_EXTENDED_COMPATIBLE_ID_DESCRIPTOR) {
        return true;
    } else if (requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_VENDOR) && request == HID_REQUEST_GET_REPORT && wIndex == 0x00 && wValue == 0x0000) {
        return true;
    } else if (consoleType == UNIVERSAL && request == USB_REQUEST_CLEAR_FEATURE && (wIndex == DEVICE_EPADDR_IN || wIndex == DEVICE_EPADDR_OUT)) {
        return true;
    }
    return false;
}
bool cleared_input = false;
bool cleared_output = false;
uint16_t controlRequest(const uint8_t requestType, const uint8_t request, const uint16_t wValue, const uint16_t wIndex, const uint16_t wLength, void *requestBuffer) {
    if (requestType == (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS)) {
        if (request == COMMAND_REBOOT) {
            reboot();
        }
        if (request == COMMAND_JUMP_BOOTLOADER) {
            bootloader();
        }
    }

    if (requestType == (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS) && request == USB_REQUEST_GET_INTERFACE) {
        ((uint8_t *)requestBuffer)[0] = 0;
        if (consoleType == UNIVERSAL && windows_or_xbox_one) {
            printf("XBOX ONE! \n");
            consoleType = XBOXONE;
            reset_usb();
            return 0;
        }
        return 1;
    }
    if (requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS) && request == USB_REQUEST_SET_INTERFACE) {
        return 0;
    }
    if (consoleType != XBOX360 && requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_VENDOR) && request == 0x81) {
        consoleType = XBOX360;
        reset_usb();
        printf("XBOX360!\n");
        return 0;
    }
#ifdef KV_KEY_1
    switch (request) {
        case 0x81:
            uint8_t serial[0x0B];
            read_serial(serial, sizeof(serial));
            xsm3_set_serial(serial);
            xsm3_initialise_state();
            xsm3_set_identification_data(xsm3_id_data_ms_controller);
            xsm3_import_kv_keys(kv_key_1, kv_key_2);  // you must fetch these from your own console!
            memcpy(requestBuffer, xsm3_id_data_ms_controller, sizeof(xsm3_id_data_ms_controller));
            return sizeof(xsm3_id_data_ms_controller);
        case 0x82:
            xsm3_do_challenge_init((uint8_t *)requestBuffer);
            return 0;
        case 0x87:
            xsm3_do_challenge_verify((uint8_t *)requestBuffer);
            return 0;
        case 0x84:
            return 0;
        case 0x83:
            memcpy(requestBuffer, xsm3_challenge_response, sizeof(xsm3_challenge_response));
            return sizeof(xsm3_challenge_response);
        case 0x86:
            short state = 2;  // 1 = in-progress, 2 = complete
            memcpy(requestBuffer, &state, sizeof(state));
            return sizeof(state);
    }
#endif
    if (requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS)) {
        switch (request) {
            case COMMAND_GET_EXTENSION_WII:
                if (!lastWiiWasSuccessful) {
                    return 0;
                }
                memcpy(requestBuffer, &wiiControllerType, sizeof(wiiControllerType));
                return sizeof(wiiControllerType);
            case COMMAND_GET_EXTENSION_PS2:
                if (!lastPS2WasSuccessful) {
                    return 0;
                }
                memcpy(requestBuffer, &ps2ControllerType, sizeof(ps2ControllerType));
                return sizeof(ps2ControllerType);
            case COMMAND_READ_WII:
                if (!lastWiiWasSuccessful) {
                    return 0;
                }
                memcpy(requestBuffer, &lastSuccessfulWiiPacket, wiiBytes);
                return wiiBytes;
            case COMMAND_READ_PS2:
                if (!lastPS2WasSuccessful) {
                    return 0;
                }
                memcpy(requestBuffer, &lastSuccessfulPS2Packet, sizeof(lastSuccessfulPS2Packet));
                return sizeof(lastSuccessfulPS2Packet);
            case COMMAND_READ_DJ_LEFT:
                if (!lastTurntableWasSuccessfulLeft) {
                    return 0;
                }
                memcpy(requestBuffer, &lastSuccessfulTurntablePacketLeft, sizeof(lastSuccessfulTurntablePacketLeft));
                return sizeof(lastSuccessfulTurntablePacketLeft);
            case COMMAND_READ_DJ_RIGHT:
                if (!lastTurntableWasSuccessfulRight) {
                    return 0;
                }
                memcpy(requestBuffer, &lastSuccessfulTurntablePacketRight, sizeof(lastSuccessfulTurntablePacketRight));
                return sizeof(lastSuccessfulTurntablePacketRight);
            case COMMAND_READ_GH5:
                if (!lastGH5WasSuccessful) {
                    return 0;
                }
                memcpy(requestBuffer, &lastSuccessfulGH5Packet, sizeof(lastSuccessfulGH5Packet));
                return sizeof(lastSuccessfulGH5Packet);

            case COMMAND_READ_GHWT:
                if (!lastGHWTWasSuccessful) {
                    return 0;
                }
                memcpy(requestBuffer, &lastTap, sizeof(lastTap));
                return sizeof(lastTap);
            case COMMAND_READ_ANALOG: {
                uint8_t pin = wValue & 0xff;
                uint8_t mask = (wValue >> 8);
                uint16_t response = adc_read(pin, mask);
                memcpy(requestBuffer, &response, sizeof(response));
                return sizeof(response);
            }
            case COMMAND_READ_DIGITAL: {
                uint8_t port = wValue & 0xff;
                uint8_t mask = (wValue >> 8);
                uint8_t response = digital_read(port, mask);
                memcpy(requestBuffer, &response, sizeof(response));
                return sizeof(response);
            }
            case COMMAND_READ_CONFIG: {
                if (wValue > sizeof(config)) {
                    return 0;
                }
                uint16_t size = sizeof(config) - wValue;
                if (size > 64) size = 64;
                memcpy_P(requestBuffer, config + wValue, size);
                return size;
            }
            case COMMAND_READ_BOARD:
                memcpy_P(requestBuffer, board, sizeof(board));
                return sizeof(board);
            case COMMAND_READ_F_CPU:
                memcpy_P(requestBuffer, f_cpu_descriptor_str, sizeof(f_cpu_descriptor_str));
                return sizeof(f_cpu_descriptor_str);
            case HID_REQUEST_GET_PROTOCOL:
                ((uint8_t *)requestBuffer)[0] = protocol_mode;
                return 1;
            case HID_REQUEST_GET_IDLE:
                ((uint8_t *)requestBuffer)[0] = idle_rate;
                return 1;
            case HID_REQUEST_GET_REPORT:
                if (wValue == 0x0300) {
                    if (consoleType == PS3) {
                        memcpy_P(requestBuffer, ps3_init, sizeof(ps3_init));
                        return sizeof(ps3_init);
                    } else if (consoleType == UNIVERSAL) {
                        consoleType = PS3;
                        printf("PS3!\n");
                        reset_usb();
                        return 0;
                    }
                }
                return 0;
        }
    } else if (request == HID_REQUEST_SET_REPORT && wValue == 0x03F4 && requestType == (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS)) {
        consoleType = PS2;
        printf("PS2!\n");
        return 0;
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
        } else if (consoleType == XBOX360 && request == HID_REQUEST_GET_REPORT && wIndex == INTERFACE_ID_Device && wValue == 0x0000) {
            memcpy_P(requestBuffer, XBOX_ID, sizeof(XBOX_ID));
            return sizeof(XBOX_ID);
        }
    } else if (requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_DEVICE | USB_SETUP_TYPE_VENDOR) && request == REQ_GET_OS_FEATURE_DESCRIPTOR && wIndex == DESC_EXTENDED_COMPATIBLE_ID_DESCRIPTOR) {
        memcpy_P(requestBuffer, &DevCompatIDs, sizeof(OS_COMPATIBLE_ID_DESCRIPTOR));
        if (consoleType == XBOXONE) {
            memcpy_P(requestBuffer, &DevCompatIDsOne, sizeof(OS_COMPATIBLE_ID_DESCRIPTOR));
            return DevCompatIDsOne.TotalLength;
        } else if (consoleType == XBOX360) {
            OS_COMPATIBLE_ID_DESCRIPTOR *compat = (OS_COMPATIBLE_ID_DESCRIPTOR *)requestBuffer;
            compat->TotalSections = 2;
            compat->TotalLength = sizeof(OS_COMPATIBLE_ID_DESCRIPTOR);
            return sizeof(OS_COMPATIBLE_ID_DESCRIPTOR);
        } else if (consoleType == UNIVERSAL && WINDOWS_USES_XINPUT) {
            windows_or_xbox_one = true;
        }
        return DevCompatIDs.TotalLength;
    } else if (request == HID_REQUEST_SET_PROTOCOL && requestType == (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS)) {
        protocol_mode = (uint8_t)wValue;
        return 0;
    } else if (request == HID_REQUEST_SET_IDLE && requestType == (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS)) {
        idle_rate = (wValue >> 8) & 0xff;
        set_idle = true;
        return 0;
    } else if (consoleType == UNIVERSAL && request == USB_REQUEST_CLEAR_FEATURE && (wIndex == DEVICE_EPADDR_IN || wIndex == DEVICE_EPADDR_OUT)) {
        // Switch clears a halt on both endpoints
        cleared_input |= wIndex == DEVICE_EPADDR_IN;
        cleared_output |= wIndex == DEVICE_EPADDR_OUT;
        if (cleared_input && cleared_output) {
            consoleType = SWITCH;
            printf("Switch!\n");
            reset_usb();
            return 0;
        }
    } else if (request == HID_REQUEST_SET_REPORT && requestType == (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS)) {
        hidInterrupt((uint8_t *)requestBuffer, wLength);
        return 0;
    }
    return 0;
}
uint8_t xbox_players[] = {
    0,  // 0x00	 All off
    0,  // 0x01	 All blinking
    1,  // 0x02	 1 flashes, then on
    2,  // 0x03	 2 flashes, then on
    3,  // 0x04	 3 flashes, then on
    4,  // 0x05	 4 flashes, then on
    1,  // 0x06	 1 on
    2,  // 0x07	 2 on
    3,  // 0x08	 3 on
    4,  // 0x09	 4 on
    0,  // 0x0A	 Rotating (e.g. 1-2-4-3)
    0,  // 0x0B	 Blinking*
    0,  // 0x0C	 Slow blinking*
    0,  // 0x0D	 Alternating (e.g. 1+4-2+3), then back to previous*
};
void handlePlayer(uint8_t player) {
    // Should be straight-forward enought to just have the gui generate if statements here, that write straight to the led array
}
void handleRumble(uint8_t rumble_left, uint8_t rumble_right) {
    // Should be straight-forward enought to just have the gui generate if statements here, that write straight to the led array
}
void hidInterrupt(const uint8_t *data, uint8_t len) {
    uint8_t id = data[0];
    if (consoleType == XBOX360) {
        while (len) {
            uint8_t size = data[1];
            len -= size;
            if (id == XBOX_LED_ID) {
                uint8_t led = data[2];
                uint8_t player = xbox_players[led];
                handlePlayer(player);
            } else if (id == XBOX_RUMBLE_ID) {
                uint8_t rumble_left = data[3];
                uint8_t rumble_right = data[4];
                handleRumble(rumble_left, rumble_right);
            }
        }
        data += len;
    } else if (consoleType == XBOXONE) {
        if (xbox_one_state == Waiting1) {
            xbox_one_state = Ident1;
        } else if (xbox_one_state == Waiting2) {
            xbox_one_state = Ident2;
        } else if (xbox_one_state == Waiting3) {
            xbox_one_state = Ident3;
        } else if (xbox_one_state == Waiting4) {
            xbox_one_state = Ident4;
        } else if (xbox_one_state == Waiting5) {
            xbox_one_state = Ident5;
        } else if (xbox_one_state == Auth) {
            if (data[0]==6 && len==6 && data[3]==2 && data[4]==1 && data[5]==0) {
                xbox_one_state = Ready;
                printf("Auth Success!\n");
            } else {
                fromConsoleLen = len;
                memcpy(fromConsole, data, len);
            }
        }
    } else {
        uint8_t *data = (uint8_t *)data;
        if (id == PS3_LED_ID) {
            uint8_t player = (data[2] >> 2);
            handlePlayer(player);
#if DEVICE_TYPE == DJ_HERO_TURNTABLE
        } else if (id == DJ_LED_ID) {
            uint8_t euphoria_on = data[2] * 0xFF;
            handleRumble(euphoria_on, euphoria_on);
#endif
        } else if (id == SANTROLLER_PS3_ID) {
            uint8_t rumble_left = data[3];
            uint8_t rumble_right = data[4];
            handleRumble(rumble_left, rumble_right);
        } else if (id == COMMAND_SET_DETECT) {
            uint8_t enabled = data[2];
            uint8_t r2_value = data[3];
            overrideR2 = enabled > 0;
            overriddenR2 = r2_value;
        }
    }
}
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
            if (consoleType == SWITCH) {
                dev->idVendor = HORI_VID;
                dev->idProduct = HORI_POKKEN_TOURNAMENT_DX_PRO_PAD_PID;
            } else if (consoleType == XBOX360) {
                dev->idProduct = ARDWIINO_360_PID;
            }
#ifdef WII_TYPE
            else if (consoleType == WII_RB) {
                dev->idVendor = WII_RB_VID;
                dev->idProduct = WII_TYPE;
            }
#endif
#ifdef PS3_TYPE
            else if (consoleType == PS3) {
                dev->idVendor = SONY_VID;
                dev->idProduct = PS3_TYPE;
            }
#endif
#ifdef XBOX_ONE_VID
            else if (consoleType == XBOXONE) {
                dev->idVendor = XBOX_ONE_VID;
                dev->idProduct = XBOX_ONE_PID;
                dev->bDeviceClass = 0xFF;
                dev->bDeviceSubClass = 0xFF;
                dev->bDeviceProtocol = 0xFF;
            }
#endif
            break;
        }
        case USB_DESCRIPTOR_CONFIGURATION: {
            if (consoleType == XBOXONE) {
                size = sizeof(XBOX_ONE_CONFIGURATION_DESCRIPTOR);
                memcpy_P(descriptorBuffer, &XBOXOneConfigurationDescriptor, size);
            } else if (consoleType == XBOX360) {
                size = sizeof(XBOX_360_CONFIGURATION_DESCRIPTOR);
                memcpy_P(descriptorBuffer, &XBOX360ConfigurationDescriptor, size);
            } else if (consoleType == MIDI) {
                size = sizeof(MIDI_CONFIGURATION_DESCRIPTOR);
                memcpy_P(descriptorBuffer, &MIDIConfigurationDescriptor, size);
            } else {
                size = sizeof(UNIVERSAL_CONFIGURATION_DESCRIPTOR);
                memcpy_P(descriptorBuffer, &UniversalConfigurationDescriptor, size);
                UNIVERSAL_CONFIGURATION_DESCRIPTOR *desc = (UNIVERSAL_CONFIGURATION_DESCRIPTOR *)descriptorBuffer;
                if (consoleType != UNIVERSAL) {
                    // Strip out all the extra interfaces used for configuring / x360 compat, the wii doesn't like em
                    desc->Config.bNumInterfaces = 1;
                    desc->Config.wTotalLength = sizeof(HID_CONFIGURATION_DESCRIPTOR);
                }
            }
            break;
        }
        case HID_DESCRIPTOR_REPORT: {
            read_hid_report_descriptor = true;
            const void *address;
#if SUPPORTS_KEYBOARD
            address = keyboard_mouse_descriptor;
            size = sizeof(keyboard_mouse_descriptor);
#else
            address = ps3_descriptor;
            size = sizeof(ps3_descriptor);
#endif
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