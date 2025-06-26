#include "descriptors.h"

#include "Usb.h"
#include "commands.h"
#include "config.h"
#include "controllers.h"
#include "hid.h"
#include "io.h"
#include "pin_funcs.h"
#include "shared_main.h"
#include "usbhid.h"
#include "util.h"
#include "xsm3/xsm3.h"

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
#if DEVICE_TYPE_IS_GAMEPAD
    bcdDevice : USB_VERSION_BCD(DEVICE_TYPE, 0, 0),
#else
    bcdDevice : USB_VERSION_BCD(0, 0, 0),
#endif
    iManufacturer : 0x01,
    iProduct : 0x02,
    iSerialNumber : 0x03,
    bNumConfigurations : 1
};
#if DEVICE_TYPE_IS_GAMEPAD
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
    InterfaceGamepad : {
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
    GamepadDescriptor : {
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

    InterfaceAudio : {
        bLength : sizeof(USB_INTERFACE_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_INTERFACE,
        bInterfaceNumber : INTERFACE_ID_Padding,
        bAlternateSetting : 0x00,
        bNumEndpoints : 4,
        bInterfaceClass : 0xFF,
        bInterfaceSubClass : 0x5D,
        bInterfaceProtocol : 0x03,
        iInterface : 0
    },
    AudioDescriptor : {0x1B, 0x21, 0x00, 0x01, 0x01, 0x01, XINPUT_MIC_IN, 0x40, 0x01, XINPUT_AUDIO_OUT,
                       0x20, 0x16, XINPUT_UNK_IN, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16,
                       XINPUT_UNK_OUT, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    ReportINEndpoint21 : {
        bLength : sizeof(USB_ENDPOINT_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_ENDPOINT,
        bEndpointAddress : XINPUT_MIC_IN,
        bmAttributes : (USB_TRANSFER_TYPE_INTERRUPT | ENDPOINT_TATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        wMaxPacketSize : 0x20,
        bInterval : 2,
    },
    ReportOUTEndpoint22 : {
        bLength : sizeof(USB_ENDPOINT_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_ENDPOINT,
        bEndpointAddress : XINPUT_AUDIO_OUT,
        bmAttributes : (USB_TRANSFER_TYPE_INTERRUPT | ENDPOINT_TATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        wMaxPacketSize : 0x20,
        bInterval : 4,
    },
    ReportINEndpoint23 : {
        bLength : sizeof(USB_ENDPOINT_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_ENDPOINT,
        bEndpointAddress : XINPUT_UNK_IN,
        bmAttributes : (USB_TRANSFER_TYPE_INTERRUPT | ENDPOINT_TATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        wMaxPacketSize : 0x20,
        bInterval : 0x40,
    },
    ReportOUTEndpoint24 : {
        bLength : sizeof(USB_ENDPOINT_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_ENDPOINT,
        bEndpointAddress : XINPUT_UNK_OUT,
        bmAttributes : (USB_TRANSFER_TYPE_INTERRUPT | ENDPOINT_TATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        wMaxPacketSize : 0x20,
        bInterval : 0x10,
    },
    InterfacePluginModule : {
        bLength : sizeof(USB_INTERFACE_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_INTERFACE,
        bInterfaceNumber : INTERFACE_ID_Config,
        bAlternateSetting : 0x00,
        bNumEndpoints : 1,
        bInterfaceClass : 0xFF,
        bInterfaceSubClass : 0x5D,
        bInterfaceProtocol : 0x02,
        iInterface : 0
    },
    PluginModuleDescriptor : {
        0x09, 0x21, 0x00, 0x01, 0x01, 0x22, XINPUT_PLUGIN_MODULE_IN, 0x07, 0x00},
    ReportINEndpoint31 : {
        bLength : sizeof(XBOX360ConfigurationDescriptor.ReportINEndpoint31),
        bDescriptorType : USB_DESCRIPTOR_ENDPOINT,
        bEndpointAddress : XINPUT_PLUGIN_MODULE_IN,
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
    SecurityDescriptor : {0x06, 0x41, 0x00, 0x01, 0x01, 0x03},
};
const PROGMEM XBOX_360_CONFIGURATION_MULTI_DESCRIPTOR XBOX360MultiConfigurationDescriptor = {
    Config : {
        bLength : sizeof(USB_CONFIGURATION_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_CONFIGURATION,
        wTotalLength : sizeof(XBOX_360_CONFIGURATION_MULTI_DESCRIPTOR),
        bNumInterfaces : 4,
        bConfigurationValue : 1,
        iConfiguration : NO_DESCRIPTOR,
        bmAttributes :
            (USB_CONFIG_ATTRIBUTE_RESERVED | USB_CONFIG_ATTRIBUTE_REMOTEWAKEUP),
        bMaxPower : USB_CONFIG_POWER_MA(500)
    },
    InterfaceGamepad : {
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
    GamepadDescriptor : {
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

    InterfaceGamepad2 : {
        bLength : sizeof(USB_INTERFACE_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_INTERFACE,
        bInterfaceNumber : INTERFACE_ID_Padding,
        bAlternateSetting : 0x00,
        bNumEndpoints : 2,
        bInterfaceClass : 0xFF,
        bInterfaceSubClass : 0x5D,
        bInterfaceProtocol : 0x01,
        iInterface : NO_DESCRIPTOR
    },
    GamepadDescriptor2 : {
        bLength : sizeof(XBOX_ID_DESCRIPTOR),
        bDescriptorType : 0x21,
        reserved : {0x10, 0x01},
        subtype : XINPUT_GAMEPAD,
        reserved2 : 0x25,
        bEndpointAddressIn : XINPUT_MIC_IN,
        bMaxDataSizeIn : 0x14,
        reserved3 : {0x03, 0x03, 0x03, 0x04, 0x13},
        bEndpointAddressOut : XINPUT_AUDIO_OUT,
        bMaxDataSizeOut : 0x08,
        reserved4 : {0x03, 0x03},
    },
    ReportINEndpoint21 : {
        bLength : sizeof(USB_ENDPOINT_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_ENDPOINT,
        bEndpointAddress : XINPUT_MIC_IN,
        bmAttributes : (USB_TRANSFER_TYPE_INTERRUPT | ENDPOINT_TATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        wMaxPacketSize : 0x20,
        bInterval : 1,
    },
    ReportOUTEndpoint22 : {
        bLength : sizeof(USB_ENDPOINT_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_ENDPOINT,
        bEndpointAddress : XINPUT_AUDIO_OUT,
        bmAttributes : (USB_TRANSFER_TYPE_INTERRUPT | ENDPOINT_TATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        wMaxPacketSize : 0x20,
        bInterval : 1,
    },
    InterfaceGamepad3 : {
        bLength : sizeof(USB_INTERFACE_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_INTERFACE,
        bInterfaceNumber : INTERFACE_ID_Config,
        bAlternateSetting : 0x00,
        bNumEndpoints : 2,
        bInterfaceClass : 0xFF,
        bInterfaceSubClass : 0x5D,
        bInterfaceProtocol : 0x01,
        iInterface : NO_DESCRIPTOR
    },
    GamepadDescriptor3 : {
        bLength : sizeof(XBOX_ID_DESCRIPTOR),
        bDescriptorType : 0x21,
        reserved : {0x10, 0x01},
        subtype : XINPUT_DRUMS,
        reserved2 : 0x25,
        bEndpointAddressIn : XINPUT_UNK_IN,
        bMaxDataSizeIn : 0x14,
        reserved3 : {0x03, 0x03, 0x03, 0x04, 0x13},
        bEndpointAddressOut : XINPUT_UNK_OUT,
        bMaxDataSizeOut : 0x08,
        reserved4 : {0x03, 0x03},
    },
    ReportINEndpoint31 : {
        bLength : sizeof(USB_ENDPOINT_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_ENDPOINT,
        bEndpointAddress : XINPUT_UNK_IN,
        bmAttributes : (USB_TRANSFER_TYPE_INTERRUPT | ENDPOINT_TATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        wMaxPacketSize : 0x20,
        bInterval : 1,
    },
    ReportOUTEndpoint32 : {
        bLength : sizeof(USB_ENDPOINT_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_ENDPOINT,
        bEndpointAddress : XINPUT_UNK_OUT,
        bmAttributes : (USB_TRANSFER_TYPE_INTERRUPT | ENDPOINT_TATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        wMaxPacketSize : 0x20,
        bInterval : 1,
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
    SecurityDescriptor : {0x06, 0x41, 0x00, 0x01, 0x01, 0x03},
};

const PROGMEM GH_ARCADE_CONFIGURATION_DESCRIPTOR GHArcadeConfigurationDescriptor{
    Config : {
        bLength : sizeof(USB_CONFIGURATION_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_CONFIGURATION,
        wTotalLength : sizeof(GH_ARCADE_CONFIGURATION_DESCRIPTOR),
        bNumInterfaces : 2,
        bConfigurationValue : 1,
        iConfiguration : NO_DESCRIPTOR,
        bmAttributes :
            (USB_CONFIG_ATTRIBUTE_RESERVED | USB_CONFIG_ATTRIBUTE_REMOTEWAKEUP),
        bMaxPower : USB_CONFIG_POWER_MA(500)
    },
    // UnknownDescriptor1 : {0x03, 0x09, 0x00},
    InterfaceHID : {
        bLength : sizeof(USB_INTERFACE_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_INTERFACE,
        bInterfaceNumber : INTERFACE_ID_Device,
        bAlternateSetting : 0,
        bNumEndpoints : 1,
        bInterfaceClass : HID_INTF,
        bInterfaceSubClass : USB_HID_PROTOCOL_NONE,
        bInterfaceProtocol : USB_HID_PROTOCOL_NONE,
        iInterface : 5
    },
    HIDDescriptor : {
        bLength : sizeof(USB_HID_DESCRIPTOR),
        bDescriptorType : HID_DESCRIPTOR_HID,
        bcdHID : USB_VERSION_BCD(1, 0, 0),
        bCountryCode : 0x00,
        bNumDescriptors : 1,
        bDescrType : HID_DESCRIPTOR_REPORT,
        wDescriptorLength : sizeof(gha_descriptor)
    },
    EndpointInHID : {
        bLength : sizeof(USB_ENDPOINT_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_ENDPOINT,
        bEndpointAddress : DEVICE_EPADDR_IN,
        bmAttributes :
            (USB_TRANSFER_TYPE_INTERRUPT | ENDPOINT_TATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        wMaxPacketSize : 0x40,
        bInterval : 1
    },
    InterfaceVendor : {
        bLength : sizeof(USB_INTERFACE_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_INTERFACE,
        bInterfaceNumber : INTERFACE_ID_Padding,
        bAlternateSetting : 0x00,
        bNumEndpoints : 3,
        bInterfaceClass : 0xFF,
        bInterfaceSubClass : 0x01,
        bInterfaceProtocol : 0xFF,
        iInterface : 6
    },
    ReportINEndpoint21 : {
        bLength : sizeof(USB_ENDPOINT_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_ENDPOINT,
        bEndpointAddress : 0x82,
        bmAttributes : (USB_TRANSFER_TYPE_INTERRUPT | ENDPOINT_TATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        wMaxPacketSize : 0x40,
        bInterval : 1,
    },
    ReportOUTEndpoint22 : {
        bLength : sizeof(USB_ENDPOINT_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_ENDPOINT,
        bEndpointAddress : 0x03,
        bmAttributes : (USB_TRANSFER_TYPE_INTERRUPT | ENDPOINT_TATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        wMaxPacketSize : 0x40,
        bInterval : 1,
    },
    ReportINEndpoint23 : {
        bLength : sizeof(USB_ENDPOINT_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_ENDPOINT,
        bEndpointAddress : 0x84,
        bmAttributes : (USB_TRANSFER_TYPE_INTERRUPT | ENDPOINT_TATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        wMaxPacketSize : 0x40,
        bInterval : 1,
    },
};

const PROGMEM OG_XBOX_CONFIGURATION_DESCRIPTOR OGXBOXConfigurationDescriptor = {
    Config : {
        bLength : sizeof(USB_CONFIGURATION_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_CONFIGURATION,
        wTotalLength : sizeof(OG_XBOX_CONFIGURATION_DESCRIPTOR),
        bNumInterfaces : 1,
        bConfigurationValue : 1,
        iConfiguration : NO_DESCRIPTOR,
        bmAttributes :
            (USB_CONFIG_ATTRIBUTE_RESERVED | USB_CONFIG_ATTRIBUTE_REMOTEWAKEUP),
        bMaxPower : USB_CONFIG_POWER_MA(500)
    },
    InterfaceGamepad : {
        bLength : sizeof(USB_INTERFACE_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_INTERFACE,
        bInterfaceNumber : INTERFACE_ID_Device,
        bAlternateSetting : 0x00,
        bNumEndpoints : 2,
        bInterfaceClass : 0x58,
        bInterfaceSubClass : 0x42,
        bInterfaceProtocol : 0,
        iInterface : NO_DESCRIPTOR
    },
    ReportINEndpoint11 : {
        bLength : sizeof(USB_ENDPOINT_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_ENDPOINT,
        bEndpointAddress : DEVICE_EPADDR_IN,
        bmAttributes : USB_TRANSFER_TYPE_INTERRUPT | ENDPOINT_TATTR_NO_SYNC | ENDPOINT_USAGE_DATA,
        wMaxPacketSize : 0x20,
        bInterval : 4,
    },
    ReportOUTEndpoint12 : {
        bLength : sizeof(USB_ENDPOINT_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_ENDPOINT,
        bEndpointAddress : DEVICE_EPADDR_OUT,
        bmAttributes : USB_TRANSFER_TYPE_INTERRUPT | ENDPOINT_TATTR_NO_SYNC | ENDPOINT_USAGE_DATA,
        wMaxPacketSize : 0x20,
        bInterval : 4,
    }
};
#endif

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
#if DEVICE_TYPE_IS_KEYBOARD
        wDescriptorLength : sizeof(keyboard_mouse_descriptor)
#else
        wDescriptorLength : sizeof(ps3_instrument_descriptor)
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
    InterfaceExtra : {
        bLength : sizeof(USB_INTERFACE_DESCRIPTOR),
        bDescriptorType : USB_DESCRIPTOR_INTERFACE,
        bInterfaceNumber : INTERFACE_ID_Padding,
        bAlternateSetting : 0x00,
        bNumEndpoints : 0,
        bInterfaceClass : 0xff,
        bInterfaceSubClass : 0xff,
        bInterfaceProtocol : 0xff,
        iInterface : NO_DESCRIPTOR
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
    InterfaceSecurity : {
        bLength : sizeof(UniversalConfigurationDescriptor.InterfaceSecurity),
        bDescriptorType : USB_DESCRIPTOR_INTERFACE,
        bInterfaceNumber : INTERFACE_ID_XBOX_Security,
        bAlternateSetting : 0x00,
        bNumEndpoints : 0,
        bInterfaceClass : 0xFF,
        bInterfaceSubClass : 0xFD,
        bInterfaceProtocol : 0x13,
        iInterface : 4
    },
    SecurityDescriptor : {0x06, 0x41, 0x00, 0x01, 0x01, 0x03},
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
// Pro instruments use a different init flow
#if DEVICE_TYPE_IS_PRO
const PROGMEM uint8_t disabled_response[5][8] = {{0xe9, 0x00, 0x00, 0x00, 0x00, 0x02, 0x0d, 0x01},
                                                 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                                 {0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x82},
                                                 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                                 {0x21, 0x26, 0x02, PS3_FLAGS, 0x00, 0x00, 0x00, 0x00}};
const PROGMEM uint8_t enabled_response[5][8] = {{0xe9, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00},
                                                {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                                {0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x8a},
                                                {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                                {0x21, 0x26, 0x02, PS3_FLAGS, 0x00, 0x00, 0x00, 0x00}};
#else
const PROGMEM uint8_t ps3_init[] = {0x21, 0x26, 0x01, PS3_FLAGS,
                                    0x00, 0x00, 0x00, 0x00};
#endif
uint8_t ef_byte = 0;
uint8_t master_bd_addr[6];
uint8_t f5_state = 0;
const PROGMEM uint8_t ps3_feature_01[] = {
    0x00, 0x01, 0x04, 0x00, 0x07, 0x0c, 0x01, 0x02,
    0x18, 0x18, 0x18, 0x18, 0x09, 0x0a, 0x10, 0x11,
    0x12, 0x13, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
    0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x04,
    0x04, 0x04, 0x04, 0x00, 0x00, 0x04, 0x00, 0x01,
    0x02, 0x07, 0x00, 0x17, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const PROGMEM uint8_t ps3_feature_f2[] = {
    0xf2, 0xff, 0xff, 0x00,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06,  // device bdaddr
    0x00, 0x03, 0x50, 0x81, 0xd8, 0x01,
    0x8a, 0x13, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
    0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x04,
    0x04, 0x04, 0x04, 0x00, 0x00, 0x04, 0x00, 0x01,
    0x02, 0x07, 0x00, 0x17, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const PROGMEM uint8_t ps3_feature_f5[] = {
    0x01, 0x00,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,  // dummy PS3 bdaddr
    0xff, 0xf7, 0x00, 0x03, 0x50, 0x81, 0xd8, 0x01,
    0x8a, 0x13, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
    0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x04,
    0x04, 0x04, 0x04, 0x00, 0x00, 0x04, 0x00, 0x01,
    0x02, 0x07, 0x00, 0x17, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const PROGMEM uint8_t ps3_feature_f7[] = {
    0x01, 0x04, 0xc4, 0x02, 0xd6, 0x01, 0xee, 0xff,
    0x14, 0x13, 0x01, 0x02, 0xc4, 0x01, 0xd6, 0x00,
    0x00, 0x02, 0x02, 0x02, 0x00, 0x03, 0x00, 0x00,
    0x02, 0x00, 0x00, 0x02, 0x62, 0x01, 0x02, 0x01,
    0x5e, 0x00, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

const PROGMEM uint8_t ps3_feature_f8[] = {
    0x00, 0x01, 0x00, 0x00, 0x07, 0x03, 0x01, 0xb0,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x02, 0x6b, 0x02, 0x68, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// Gyro and accel calibration are in here somewhere!
const PROGMEM uint8_t ps3_feature_ef[] = {
    0x00, 0xef, 0x04, 0x00, 0x05, 0x03, 0x01, 0xb0,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x03,
    0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

const PROGMEM uint8_t ps4_feature_config[] = {
    0x03, 0x21, 0x27, 0x04, 0x91, PS4_TYPE, 0x2c, 0x56,
    0xa0, 0x0f, 0x3d, 0x00, 0x00, 0x04, 0x01, 0x00,
    0x00, 0x20, 0x0d, 0x0d, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

AuthPageSizeReport ps4_pagesize_report = {
    type : 0xF3,
    u1 : 0x00,
    size_challenge : 0x38,
    size_response : 0x38,
    u4 : {0x00, 0x00, 0x00, 0x00}
};

uint8_t idle_rate;
uint8_t protocol_mode = HID_RPT_PROTOCOL;
bool controlRequestValid(const uint8_t requestType, const uint8_t request, const uint16_t wValue, const uint16_t wIndex, const uint16_t wLength) {
    // printf("%02x %04x %04x %04x %04x\r\n", requestType, request, wValue, wIndex, wLength);
    if (consoleType == UNIVERSAL && requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_VENDOR) && request == 0x81) {
        return true;
    }
    if (consoleType != OG_XBOX && requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_VENDOR) && request == 6 && wValue == 0x4200) {
        return true;
    }
    if (consoleType == OG_XBOX && requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_VENDOR)) {
        if (request == 6 && wValue == 0x4200) {
            return true;
        }
        if (request == 1 && wValue == 0x0100) {
            return true;
        }
        if (request == 1 && wValue == 0x0200) {
            return true;
        }
    }
    // Doing this actually *breaks* xb360 on pro micros.
    // But its necessary on the pico :/
#if SUPPORTS_PICO
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
    if (requestType == (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS)) {
        switch (request) {
            case HID_REQUEST_SET_PROTOCOL:
            case HID_REQUEST_SET_IDLE:
            case HID_REQUEST_SET_REPORT:
                return true;
        }

        if (request >= COMMAND_REBOOT && request < MAX) {
            return true;
        }
    }
    if (requestType == (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS) && request == USB_REQUEST_GET_INTERFACE) {
        return true;
    }
    if (requestType == (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS) && request == USB_REQUEST_GET_STATUS) {
        return true;
    }
    if (requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS) && request == USB_REQUEST_SET_INTERFACE) {
        return true;
    }
    if (requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS)) {
        switch (request) {
            case HID_REQUEST_GET_PROTOCOL:
            case HID_REQUEST_GET_IDLE:
            case HID_REQUEST_GET_REPORT:
                return true;
        }
        if (request >= COMMAND_REBOOT && request < MAX) {
            return true;
        }
    } else if (requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_VENDOR)) {
        if (request == HID_REQUEST_GET_REPORT && wIndex == INTERFACE_ID_Device && wValue == VIBRATION_CAPABILITIES_WVALUE) {
            return true;
        } else if (request == REQ_GET_OS_FEATURE_DESCRIPTOR && wIndex == DESC_EXTENDED_PROPERTIES_DESCRIPTOR && wValue == INTERFACE_ID_Config) {
            return true;
        } else if (request == HID_REQUEST_GET_REPORT && wIndex == INTERFACE_ID_Device && wValue == INPUT_CAPABILITIES_WVALUE) {
            return true;
        }
    } else if (requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_DEVICE | USB_SETUP_TYPE_VENDOR) && (consoleType == WINDOWS || consoleType == XBOX360) && request == HID_REQUEST_GET_REPORT && wIndex == 0x0000 && wValue == SERIAL_NUMBER_WVALUE) {
        return true;
    } else if (requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_DEVICE | USB_SETUP_TYPE_VENDOR) && request == REQ_GET_OS_FEATURE_DESCRIPTOR && wIndex == DESC_EXTENDED_COMPATIBLE_ID_DESCRIPTOR) {
        return true;
    } else if (consoleType == UNIVERSAL && request == USB_REQUEST_CLEAR_FEATURE && (wIndex == DEVICE_EPADDR_IN || wIndex == DEVICE_EPADDR_OUT)) {
        return true;
    }
    return false;
}
uint8_t ps3_id_id = 4;
bool cleared_input = false;
bool cleared_output = false;
uint16_t controlRequest(const uint8_t requestType, const uint8_t request, const uint16_t wValue, const uint16_t wIndex, const uint16_t wLength, uint8_t *requestBuffer, bool *status) {
    // printf("%02x %04x %04x %04x %04x\r\n", requestType, request, wValue, wIndex, wLength);
#if DEVICE_TYPE_IS_GAMEPAD
    if (consoleType != OG_XBOX && requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_VENDOR) && request == 6 && wValue == 0x4200) {
        consoleType = OG_XBOX;
        reset_usb();
        return 0;
    }
    if (consoleType == OG_XBOX && requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_VENDOR)) {
        if (request == 6 && wValue == 0x4200) {
            memcpy_P(requestBuffer, &DukeXIDDescriptor, sizeof(DukeXIDDescriptor));
            return sizeof(DukeXIDDescriptor);
        }
        if (request == 1 && wValue == 0x0100) {
            memcpy_P(requestBuffer, &DukeXIDInputCapabilities, sizeof(DukeXIDInputCapabilities));
            return sizeof(DukeXIDInputCapabilities);
        }
        if (request == 1 && wValue == 0x0200) {
            memcpy_P(requestBuffer, &DukeXIDVibrationCapabilities, sizeof(DukeXIDVibrationCapabilities));
            return sizeof(DukeXIDVibrationCapabilities);
        }
    }
#endif

    if (requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_VENDOR) && request == 0x83) {
        if (xbox_360_state == Auth1) {
            xbox_360_state = Auth2;
        } else if (xbox_360_state == Auth2) {
            xbox_360_state = Auth3;
        } else if (xbox_360_state == Auth3) {
            xbox_360_state = Authenticated;
            handle_auth_led();
            if (consoleType != XBOX360) {
                consoleType = XBOX360;
                reset_after_360 = millis() + 100;
            }
        }
    }
    if (seen_windows_xb1 && !(requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_DEVICE | USB_SETUP_TYPE_VENDOR) && request == REQ_GET_OS_FEATURE_DESCRIPTOR && wIndex == DESC_EXTENDED_COMPATIBLE_ID_DESCRIPTOR)) {
        seen_windows = true;
    }
    if (requestType == (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS)) {
        if (request == COMMAND_REBOOT) {
            reboot();
        }
        if (request == COMMAND_JUMP_BOOTLOADER) {
            bootloader();
        }
        if (request >= COMMAND_REBOOT && request < MAX) {
            bool success;
            return handle_serial_command(request, wValue, requestBuffer, &success);
        }
    }

    if (requestType == (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS) && request == USB_REQUEST_GET_INTERFACE) {
        ((uint8_t *)requestBuffer)[0] = 0;
        return 1;
    }
    if (requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS) && request == USB_REQUEST_SET_INTERFACE) {
        return 0;
    }

#if DEVICE_TYPE_IS_GAMEPAD
#if SUPPORTS_PICO
    switch (request) {
        case 0x81:
            uint8_t serial[0x0C];
            read_serial(serial, sizeof(serial));
            xsm3_set_vid_pid(serial, ARDWIINO_VID, ARDWIINO_PID);
            xsm3_initialise_state();
            xsm3_set_identification_data(xsm3_id_data_ms_controller);
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
            return wLength;
        case 0x86:
            short state = 2;  // 1 = in-progress, 2 = complete
            memcpy(requestBuffer, &state, sizeof(state));
            return sizeof(state);
    }
#else
    if (consoleType == UNIVERSAL && requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_VENDOR) && request == 0x81) {
        consoleType = XBOX360;
        reset_usb();
        return 0;
    }
    switch (request) {
        case 0x81:
        case 0x82:
        case 0x87:
        case 0x84:
        case 0x83:
        case 0x86:
            return 0;
    }
#endif
#endif

    if (requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS)) {
#if DEVICE_TYPE_IS_GAMEPAD
        // PS3s request this as some form of controller id
        if ((consoleType == PS3 || consoleType == IOS_FESTIVAL) && wValue == 0x0300 && wIndex == INTERFACE_ID_Device && request == HID_REQUEST_GET_REPORT && wLength == 0x08) {
            // Pro instruments use a different init flow
#if DEVICE_TYPE_IS_PRO
            if (proButtonsEnabled) {
                memcpy_P(requestBuffer, enabled_response[ps3_id_id], sizeof(enabled_response[ps3_id_id]));
            } else {
                memcpy_P(requestBuffer, disabled_response[ps3_id_id], sizeof(disabled_response[ps3_id_id]));
            }
            ps3_id_id++;
            if (ps3_id_id > 4) ps3_id_id = 4;
            return 8;
#else
            memcpy_P(requestBuffer, ps3_init, sizeof(ps3_init));
            return sizeof(ps3_init);
#endif
        }
        // Fakemote sends this, so we know to jump to PS3 mode
        if ((consoleType == UNIVERSAL || consoleType == WINDOWS) && wValue == 0x03f2 && (wIndex == INTERFACE_ID_Device || wIndex == INTERFACE_ID_Config) && request == HID_REQUEST_GET_REPORT && wLength == 0x11) {
            if (RPCS3_COMPAT) {
                consoleType = PS3;
                reset_usb();
            }
        }
        // RPCS3 sends this. Jump any devices with RPCS3 compat enabled
        if ((consoleType == UNIVERSAL || consoleType == WINDOWS) && wValue == 0x03f2 && wIndex == INTERFACE_ID_Config && request == HID_REQUEST_GET_REPORT) {
            if (RPCS3_COMPAT) {
                consoleType = PS3;
                reset_usb();
            }
            seen_rpcs3 = true;
        }
        if ((consoleType == PS3 || consoleType == IOS_FESTIVAL) && wValue == 0x03f8 && wIndex == INTERFACE_ID_Device && request == HID_REQUEST_GET_REPORT) {
            memcpy_P(requestBuffer, ps3_feature_f8, sizeof(ps3_feature_f8));
            requestBuffer[7] = ef_byte;
            return sizeof(ps3_feature_f8);
        }

        if ((consoleType == PS3 || consoleType == IOS_FESTIVAL) && wValue == 0x03f7 && wIndex == INTERFACE_ID_Device && request == HID_REQUEST_GET_REPORT) {
            memcpy_P(requestBuffer, ps3_feature_f7, sizeof(ps3_feature_f7));
            return sizeof(ps3_feature_f7);
        }
        if ((consoleType == PS3 || consoleType == IOS_FESTIVAL) && wValue == 0x03f2 && wIndex == INTERFACE_ID_Device && request == HID_REQUEST_GET_REPORT) {
            memcpy_P(requestBuffer, ps3_feature_f2, sizeof(ps3_feature_f2));
            return sizeof(ps3_feature_f2);
        }
        if ((consoleType == PS3 || consoleType == IOS_FESTIVAL) && wValue == 0x03f5 && wIndex == INTERFACE_ID_Device && request == HID_REQUEST_GET_REPORT) {
            memcpy_P(requestBuffer, ps3_feature_f5, sizeof(ps3_feature_f5));
            if (f5_state == 0) {
                /*
                 * First request, tell that the bdaddr is not the one of the PS3.
                 */
                f5_state = 1;
            } else {
                /*
                 * Next requests, tell that the bdaddr is the one of the PS3.
                 */
                memcpy(requestBuffer + 2, master_bd_addr, sizeof(master_bd_addr));
            }
            return sizeof(ps3_feature_f5);
        }
        if ((consoleType == PS3 || consoleType == IOS_FESTIVAL) && wValue == 0x03ef && wIndex == INTERFACE_ID_Device && request == HID_REQUEST_GET_REPORT) {
            memcpy_P(requestBuffer, ps3_feature_ef, sizeof(ps3_feature_ef));
            requestBuffer[7] = ef_byte;
            return sizeof(ps3_feature_ef);
        }
        // PS3 sends this for a gamepad
        if ((consoleType == PS3 || consoleType == IOS_FESTIVAL) && wValue == 0x0301 && wIndex == INTERFACE_ID_Device && request == HID_REQUEST_GET_REPORT && wLength == 0x40) {
            memcpy_P(requestBuffer, ps3_feature_01, sizeof(ps3_feature_01));
            return sizeof(ps3_feature_01);
        }
        // PS3 and PS4 send this
        if (consoleType == UNIVERSAL && wIndex == INTERFACE_ID_Device && request == HID_REQUEST_GET_REPORT && wValue == 0x0303) {
            // PS3 Drums and Guitars get used on both consoles, so we can jump straight to PS3 mode
#if DEVICE_TYPE_IS_INSTRUMENT && !SUPPORTS_PS4
            consoleType = PS3;
            reset_usb();
#else
            // the PS3 and PS4 will end up here. PS4 mode will jump back to PS3 mode on a PS3 later.
            consoleType = PS4;
            reset_usb();
#endif
        }
        if (consoleType == PS4 && wIndex == INTERFACE_ID_Device && request == HID_REQUEST_GET_REPORT) {
            switch (wValue) {
                case 0x0303:
                    seen_ps4 = true;
                    memcpy_P(requestBuffer, ps4_feature_config, sizeof(ps4_feature_config));
                    return sizeof(ps4_feature_config);

#if USB_HOST_STACK
                case GET_RESPONSE: {
                    return transfer_with_usb_controller(get_device_address_for(PS4).dev_addr, requestType, request, wValue, wIndex, wLength, requestBuffer, status);
                }
                case GET_AUTH_STATUS: {
                    return transfer_with_usb_controller(get_device_address_for(PS4).dev_addr, requestType, request, wValue, wIndex, wLength, requestBuffer, status);
                }
                case GET_AUTH_PAGE_SIZE: {
                    // Don't start the auth process until we see a controller
                    if (get_device_address_for(PS4).dev_addr == 0) {
                        return 0;
                    }
                    // Attempt to read the page size from the connected controller
                    // On a DS4, this will fail but we have the default page size report for that scenario.
                    transfer_with_usb_controller(get_device_address_for(PS4).dev_addr, (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS), HID_REQUEST_GET_REPORT, GET_AUTH_PAGE_SIZE, INTERFACE_ID_Device, sizeof(AuthPageSizeReport), (uint8_t *)&ps4_pagesize_report, NULL);
                    memcpy_P(requestBuffer, &ps4_pagesize_report, sizeof(ps4_pagesize_report));
                    return sizeof(ps4_pagesize_report);
                }
#else
                case GET_AUTH_PAGE_SIZE: {
                    memcpy_P(requestBuffer, &ps4_pagesize_report, sizeof(ps4_pagesize_report));
                    return sizeof(ps4_pagesize_report);
                }
#endif
            }
        }
        if (wValue == 0x0101 && wIndex == INTERFACE_ID_Device && request == HID_REQUEST_GET_REPORT && wLength == 0x80) {
            return tick_inputs(requestBuffer, NULL, consoleType);
        }
        if (consoleType == OG_XBOX && wValue == 0x0100 && wIndex == INTERFACE_ID_Device && request == HID_REQUEST_GET_REPORT && wLength == sizeof(OG_XBOX_REPORT)) {
            return tick_inputs(requestBuffer, NULL, consoleType);
        }
#endif
        bool test = true;
        uint8_t size = handle_serial_command(request, wValue, requestBuffer, &test);
        if (test) {
            return size;
        }
    } else if (request == HID_REQUEST_SET_REPORT && wValue == SET_CHALLENGE && requestType == (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS)) {
#if USB_HOST_STACK
        return transfer_with_usb_controller(get_device_address_for(PS4).dev_addr, requestType, request, wValue, wIndex, wLength, requestBuffer, status);
#else
        return 0;
#endif
    } else if (request == HID_REQUEST_SET_REPORT && wValue == 0x03F2 && requestType == (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS)) {
        return 1;
    } else if (request == HID_REQUEST_SET_REPORT && wValue == 0x03F4 && requestType == (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS)) {
        // pademu sends this, so we know to jump to PS3 mode
        if (consoleType == UNIVERSAL && wLength == 0x04) {
            consoleType = PS3;
            reset_usb();
        }
        return 1;
    } else if ((consoleType == PS3 || consoleType == IOS_FESTIVAL) && wValue == 0x03ef && wIndex == INTERFACE_ID_Device && request == HID_REQUEST_SET_REPORT) {
        ef_byte = requestBuffer[6];
        return 1;
    } else if ((consoleType == PS3 || consoleType == IOS_FESTIVAL) && wValue == 0x03f5 && wIndex == INTERFACE_ID_Device && request == HID_REQUEST_SET_REPORT) {
        memcpy(master_bd_addr, requestBuffer + 2, sizeof(master_bd_addr));
        printf("Master bd address set\r\n");
        return 1;
    } else if (requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_VENDOR)) {
        if (request == HID_REQUEST_GET_REPORT && wIndex == INTERFACE_ID_Device && wValue == VIBRATION_CAPABILITIES_WVALUE) {
            memcpy_P(requestBuffer, &XInputVibrationCapabilities, sizeof(XInputVibrationCapabilities));
            return sizeof(XInputVibrationCapabilities);
        } else if (request == REQ_GET_OS_FEATURE_DESCRIPTOR && wIndex == DESC_EXTENDED_PROPERTIES_DESCRIPTOR) {
            memcpy_P(requestBuffer, &ExtendedIDs, ExtendedIDs.TotalLength);
            return ExtendedIDs.TotalLength;
        } else if (request == HID_REQUEST_GET_REPORT && wIndex == INTERFACE_ID_Device && wValue == INPUT_CAPABILITIES_WVALUE) {
            memcpy_P(requestBuffer, &XInputInputCapabilities, sizeof(XInputInputCapabilities));
            return sizeof(XInputInputCapabilities);
        }
    } else if (requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_DEVICE | USB_SETUP_TYPE_VENDOR) && (consoleType == WINDOWS || consoleType == XBOX360) && request == HID_REQUEST_GET_REPORT && wIndex == 0x0000 && wValue == SERIAL_NUMBER_WVALUE) {
        uint32_t serial = micros();
        memcpy(requestBuffer, &serial, sizeof(serial));
        return sizeof(XInputSerialNumber_t);
    } else if (requestType == (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_DEVICE | USB_SETUP_TYPE_VENDOR) && request == REQ_GET_OS_FEATURE_DESCRIPTOR && wIndex == DESC_EXTENDED_COMPATIBLE_ID_DESCRIPTOR) {
        seen_windows_xb1 = true;
        memcpy_P(requestBuffer, &DevCompatIDs, sizeof(OS_COMPATIBLE_ID_DESCRIPTOR));
#if USB_HOST_STACK
        if (consoleType == XBOXONE) {
            memcpy_P(requestBuffer, &DevCompatIDsOne, sizeof(DevCompatIDsOne));
            return sizeof(DevCompatIDsOne);
        } else
#endif
            if (consoleType == WINDOWS || consoleType == XBOX360) {
            OS_COMPATIBLE_ID_DESCRIPTOR *compat = (OS_COMPATIBLE_ID_DESCRIPTOR *)requestBuffer;
            compat->TotalSections = 4;
            compat->TotalLength = sizeof(OS_COMPATIBLE_ID_DESCRIPTOR);
            return sizeof(OS_COMPATIBLE_ID_DESCRIPTOR);
        } else if (consoleType == PS3 || consoleType == WII_RB) {
            memcpy_P(requestBuffer, &DevCompatIDsPS3, sizeof(OS_COMPATIBLE_ID_DESCRIPTOR_SINGLE));
            return sizeof(OS_COMPATIBLE_ID_DESCRIPTOR_SINGLE);
        } else if (consoleType != UNIVERSAL) {
            return 0;
        }
        memcpy_P(requestBuffer, &DevCompatIDsUniversal, sizeof(DevCompatIDsUniversal));
        return sizeof(DevCompatIDsUniversal);
    } else if (request == HID_REQUEST_SET_PROTOCOL && requestType == (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS)) {
        protocol_mode = (uint8_t)wValue;
        return 0;
    } else if (request == HID_REQUEST_SET_IDLE && requestType == (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS)) {
        idle_rate = (wValue >> 8) & 0xff;
        return 0;
    } else if (consoleType == UNIVERSAL && request == USB_REQUEST_CLEAR_FEATURE && (wIndex == DEVICE_EPADDR_IN || wIndex == DEVICE_EPADDR_OUT)) {
        // Switch clears a halt on both endpoints
        cleared_input |= wIndex == DEVICE_EPADDR_IN;
        cleared_output |= wIndex == DEVICE_EPADDR_OUT;
        if (cleared_input && cleared_output) {
            consoleType = SWITCH;
            reset_usb();
            return 0;
        }
    } else if (request == HID_REQUEST_SET_REPORT && requestType == (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS)) {
        const uint8_t reportType = (wValue >> 8);
        const uint8_t reportId = (wValue & 0xFF);
        hid_set_report((uint8_t *)requestBuffer, wLength, reportType, reportId);

#if DEVICE_TYPE_IS_PRO
        if ((consoleType == PS3 || consoleType == IOS_FESTIVAL) && wValue == 0x0300 && wIndex == INTERFACE_ID_Device && wLength == 0x28) {
            // for (int i = 0; i < 0x28; i++) {
            //     printf("%02x, ", requestBuffer[i]);
            // }
            // printf("\r\n");
            switch (requestBuffer[2]) {
                case 0x89:
                    printf("MIDI data enabled\r\n");
                    proButtonsEnabled = true;
                    ps3_id_id = 0;
                    break;
                case 0x81:
                    printf("MIDI data disabled\r\n");
                    proButtonsEnabled = false;
                    ps3_id_id = 0;
                    break;
            }
        }
#endif
        return 0;
    }
    return 0;
}
bool seen_windows_desc = false;
uint16_t descriptorRequest(const uint16_t wValue,
                           const uint16_t wIndex,
                           void *descriptorBuffer) {
    const uint8_t descriptorType = (wValue >> 8);
    const uint8_t descriptorNumber = (wValue & 0xFF);
#if DEVICE_TYPE_IS_GAMEPAD
#if USB_HOST_STACK
    if (consoleType == UNIVERSAL && seen_windows_xb1 && descriptorType != HID_DESCRIPTOR_REPORT) {
        seen_windows_desc = true;
    }

    if (consoleType == UNIVERSAL && seen_windows_xb1 && descriptorType == HID_DESCRIPTOR_REPORT) {
        if (seen_windows_desc) {
            if (WINDOWS_USES_XINPUT) {
                set_console_type(WINDOWS);
            }
        } else {
            set_console_type(XBOXONE);
        }
    }
#else
    if (WINDOWS_USES_XINPUT && consoleType == UNIVERSAL && seen_windows_xb1 && descriptorType != HID_DESCRIPTOR_REPORT) {
        set_console_type(WINDOWS);
    }

#endif
#endif
    descriptor_requested = true;
    uint16_t size = NO_DESCRIPTOR;
    switch (descriptorType) {
        case USB_DESCRIPTOR_DEVICE: {
            read_device_desc = true;
            size = sizeof(USB_DEVICE_DESCRIPTOR);
            memcpy_P(descriptorBuffer, &deviceDescriptor, size);
            USB_DEVICE_DESCRIPTOR *dev = (USB_DEVICE_DESCRIPTOR *)descriptorBuffer;
#if DEVICE_TYPE_IS_GAMEPAD
            if (consoleType == SWITCH) {
                dev->idVendor = HORI_VID;
                dev->idProduct = HORI_POKKEN_TOURNAMENT_DX_PRO_PAD_PID;
            } else if (consoleType == XBOX360) {
                dev->idVendor = ARDWIINO_VID;
                dev->idProduct = ARDWIINO_PID;
            } else if (consoleType == OG_XBOX) {
                dev->idVendor = 0x045E;
                dev->idProduct = 0x0289;
            }
#if USB_HOST_STACK
            else if (consoleType == XBOXONE) {
                dev->idVendor = XBOX_ONE_CONTROLLER_VID;
                dev->idProduct = XBOX_ONE_CONTROLLER_PID;
                dev->bDeviceClass = 0xff;
                dev->bDeviceSubClass = 0x47;
                dev->bDeviceProtocol = 0xd0;
            }
#endif
            else if (consoleType == PS4) {
                dev->idVendor = PS4_VID;
                dev->idProduct = PS4_PID;
            } else if (consoleType == IOS_FESTIVAL) {
                dev->idVendor = SONY_VID;
                dev->idProduct = SONY_DS3_PID;
            }
#ifdef WII_TYPE
            else if (consoleType == WII_RB) {
                dev->idVendor = HARMONIX_VID;
                dev->idProduct = WII_TYPE;
            }
#endif
#ifdef PS3_TYPE
            else if (consoleType == PS3) {
                dev->idVendor = REDOCTANE_VID;
                dev->idProduct = PS3_TYPE;
                dev->bcdDevice = 0x0108;
            }
#elif DEVICE_TYPE == GAMEPAD
            else if (consoleType == PS3) {
                dev->idVendor = SONY_VID;
                dev->idProduct = SONY_DS3_PID;
            }
#endif
#endif
#if DEVICE_TYPE == ROCK_BAND_GUITAR || DEVICE_TYPE == GUITAR_HERO_GUITAR
            if (consoleType == FNF) {
                dev->idVendor = PDP_VID;
                dev->idProduct = XBOX_ONE_JAG_PID;
            }
#endif
            if (consoleType == ARCADE) {
                dev->idVendor = 0x0c70;
                dev->idProduct = 0x0777;
            }
            break;
        }
        case USB_DESCRIPTOR_CONFIGURATION: {
#if CONSOLE_TYPE == MIDI_ID
            size = sizeof(MIDI_CONFIGURATION_DESCRIPTOR);
            memcpy_P(descriptorBuffer, &MIDIConfigurationDescriptor, size);
#elif DEVICE_TYPE_IS_KEYBOARD
            size = sizeof(UNIVERSAL_CONFIGURATION_DESCRIPTOR);
            memcpy_P(descriptorBuffer, &UniversalConfigurationDescriptor, size);
#else

#if USB_HOST_STACK
            if (consoleType == XBOXONE) {
                size = sizeof(XBOX_ONE_CONFIGURATION_DESCRIPTOR);
                memcpy_P(descriptorBuffer, &XBOXOneConfigurationDescriptor, size);
            } else
#endif
                if (consoleType == WINDOWS || consoleType == XBOX360) {
                size = sizeof(XBOX_360_CONFIGURATION_DESCRIPTOR);
                memcpy_P(descriptorBuffer, &XBOX360ConfigurationDescriptor, size);
            } else if (consoleType == OG_XBOX) {
                size = sizeof(OG_XBOX_CONFIGURATION_DESCRIPTOR);
                memcpy_P(descriptorBuffer, &OGXBOXConfigurationDescriptor, size);
            } else if (consoleType == ARCADE) {
                size = sizeof(GH_ARCADE_CONFIGURATION_DESCRIPTOR);
                memcpy_P(descriptorBuffer, &GHArcadeConfigurationDescriptor, size);
            } else {
                size = sizeof(UNIVERSAL_CONFIGURATION_DESCRIPTOR);
                memcpy_P(descriptorBuffer, &UniversalConfigurationDescriptor, size);
                UNIVERSAL_CONFIGURATION_DESCRIPTOR *desc = (UNIVERSAL_CONFIGURATION_DESCRIPTOR *)descriptorBuffer;

                if (consoleType == PS4) {
                    desc->HIDDescriptor.wDescriptorLength = sizeof(ps4_descriptor);
                    desc->EndpointOutHID.wMaxPacketSize = 64;
                    desc->EndpointInHID.wMaxPacketSize = 64;
                } else if (consoleType == IOS_FESTIVAL) {
                    desc->HIDDescriptor.wDescriptorLength = sizeof(ps3_descriptor);
                    desc->EndpointOutHID.wMaxPacketSize = 64;
                    desc->EndpointInHID.wMaxPacketSize = 64;
#if DEVICE_TYPE_IS_INSTRUMENT
#if defined(TICK_NKRO) || defined(TICK_SIXKRO)
                } else if (consoleType == KEYBOARD_MOUSE) {
                    desc->HIDDescriptor.wDescriptorLength = sizeof(keyboard_mouse_descriptor);
#endif
#if DEVICE_TYPE == ROCK_BAND_GUITAR || DEVICE_TYPE == GUITAR_HERO_GUITAR
                } else if (consoleType == FNF) {
                    desc->HIDDescriptor.wDescriptorLength = sizeof(fnf_descriptor);
#endif
                } else {
                    desc->HIDDescriptor.wDescriptorLength = sizeof(ps3_instrument_descriptor);
                }
#else
                } else if (consoleType == PS3) {
                    desc->HIDDescriptor.wDescriptorLength = sizeof(ps3_descriptor);
                    desc->EndpointOutHID.wMaxPacketSize = 64;
                    desc->EndpointInHID.wMaxPacketSize = 64;
                } else {
                    desc->HIDDescriptor.wDescriptorLength = sizeof(ps3_instrument_descriptor);
                }
#endif
                if (consoleType == UNIVERSAL) {
                    desc->HIDDescriptor.wDescriptorLength = sizeof(pc_descriptor);
                } else {
                    // Strip out all the extra interfaces used for configuring / x360 compat, consoles dont need them
                    desc->Config.bNumInterfaces = 1;
                    desc->Config.wTotalLength = sizeof(HID_CONFIGURATION_DESCRIPTOR);
                }
            }
#endif
            break;
        }
        case HID_DESCRIPTOR_REPORT: {
            const void *address;
            seen_hid_descriptor_read = true;
#if DEVICE_TYPE_IS_KEYBOARD
            address = keyboard_mouse_descriptor;
            size = sizeof(keyboard_mouse_descriptor);
#elif DEVICE_TYPE_IS_GAMEPAD
            if (consoleType == ARCADE) {
                address = gha_descriptor;
                size = sizeof(gha_descriptor);
            } else if (consoleType == PS4) {
                address = ps4_descriptor;
                size = sizeof(ps4_descriptor);
            } else if (consoleType == IOS_FESTIVAL) {
                address = ps3_descriptor;
                size = sizeof(ps3_descriptor);
#if DEVICE_TYPE == ROCK_BAND_GUITAR || DEVICE_TYPE == GUITAR_HERO_GUITAR
            } else if (consoleType == FNF) {
                address = fnf_descriptor;
                size = sizeof(fnf_descriptor);
#endif
#if defined(TICK_NKRO) || defined(TICK_SIXKRO)
            } else if (consoleType == KEYBOARD_MOUSE) {
                address = keyboard_mouse_descriptor;
                size = sizeof(keyboard_mouse_descriptor);
#endif
            } else if (consoleType != UNIVERSAL) {
#if DEVICE_TYPE_IS_INSTRUMENT
                address = ps3_instrument_descriptor;
                size = sizeof(ps3_instrument_descriptor);
#else
                if (consoleType == PS3) {
                    address = ps3_descriptor;
                    size = sizeof(ps3_descriptor);
                } else {
                    address = ps3_instrument_descriptor;
                    size = sizeof(ps3_instrument_descriptor);
                }
#endif
            } else {
                address = pc_descriptor;
                size = sizeof(pc_descriptor);
            }
#else
            address = ps3_instrument_descriptor;
            size = sizeof(ps3_instrument_descriptor);
#endif
            memcpy_P(descriptorBuffer, address, size);
            break;
        }
        case USB_DESCRIPTOR_STRING: {
            read_any_string = true;
            const uint8_t *str;
            if (descriptorNumber == 5) {
                str = (uint8_t *)&rtStringInput;
            } else if (descriptorNumber == 6) {
                str = (uint8_t *)&rtStringLED;
            } else if (descriptorNumber == 4) {
                str = (uint8_t *)&xboxString;
            } else if (descriptorNumber < 4) {
                str = (uint8_t *)pgm_read_pointer(descriptorStrings + descriptorNumber);
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
