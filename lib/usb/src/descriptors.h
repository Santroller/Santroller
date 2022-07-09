#pragma once
#include <stdint.h>

#include "Usb.h"
#include "endpoints.h"
#include "midi.h"
#include "progmem.h"
#include "wcid.h"
#include "xbox.h"

#define USB_DESCRIPTOR_STRING_ARRAY(...)                                                                                                                             \
    {                                                                                                                                                                \
        .bLength = sizeof(uint8_t) + sizeof(uint8_t) + sizeof((uint16_t[]){__VA_ARGS__}), .bDescriptorType = USB_DESCRIPTOR_STRING, .UnicodeString = { __VA_ARGS__ } \
    }

#define USB_VERSION_BCD(Major, Minor, Revision) \
    (((Major & 0xFF) << 8) |                    \
     ((Minor & 0x0F) << 4) |                    \
     (Revision & 0x0F))

#define USB_CONFIG_POWER_MA(mA) ((mA) >> 1)

#define NO_DESCRIPTOR 0

#define LANGUAGE_ID_ENG 0x0409

#define USB_CONFIG_ATTRIBUTE_RESERVED 0x80
#define USB_CONFIG_ATTRIBUTE_SELFPOWERED 0x40
#define USB_CONFIG_ATTRIBUTE_REMOTEWAKEUP 0x20

#define HID_DESCRIPTOR_REPORT 0x22

#define ENDPOINT_TATTR_NO_SYNC (0 << 2)
#define ENDPOINT_TATTR_ASYNC (1 << 2)
#define ENDPOINT_TATTR_ADAPTIVE (2 << 2)
#define ENDPOINT_TATTR_SYNC (3 << 2)
#define ENDPOINT_USAGE_DATA (0 << 4)
#define ENDPOINT_USAGE_FEEDBACK (1 << 4)
#define ENDPOINT_USAGE_IMPLICIT_FEEDBACK (2 << 4)

typedef struct
{
    uint8_t bLength;          // Length of this descriptor.
    uint8_t bDescriptorType;  // CONFIGURATION descriptor type (USB_DESCRIPTOR_CONFIGURATION).
    uint16_t UnicodeString[]; /**< String data, as unicode characters (alternatively,
                               *   string language IDs). If normal ASCII characters are
                               *   to be used, they must be added as an array of characters
                               *   rather than a normal C string so that they are widened to
                               *   Unicode size.
                               *
                               *   Under GCC, strings prefixed with the "L" character (before
                               *   the opening string quotation mark) are considered to be
                               *   Unicode strings, and may be used instead of an explicit
                               *   array of ASCII characters on little endian devices with
                               *   UTF-16-LE \c wchar_t encoding.
                               */
} __attribute__((packed)) STRING_DESCRIPTOR;

typedef struct {
    USB_CONFIGURATION_DESCRIPTOR Config;
    USB_INTERFACE_DESCRIPTOR Interface1;
    XBOX_ID_DESCRIPTOR Interface1ID;
    USB_ENDPOINT_DESCRIPTOR ReportINEndpoint11;
    USB_ENDPOINT_DESCRIPTOR ReportOUTEndpoint12;
    USB_INTERFACE_DESCRIPTOR Interface4;
    uint8_t UnkownDescriptor4[0x06];
    USB_INTERFACE_DESCRIPTOR InterfaceConfig;
    USB_INTERFACE_DESCRIPTOR InterfaceExtra;
} __attribute__((packed)) CONFIGURATION_XBOX_PC_DESCRIPTOR;

typedef struct {
    USB_CONFIGURATION_DESCRIPTOR Config;

    // Joystick HID Interface
    USB_INTERFACE_DESCRIPTOR HID_Interface;
    USB_HID_DESCRIPTOR HID_JoystickHID;
    USB_ENDPOINT_DESCRIPTOR HID_ReportOUTEndpoint;
    USB_ENDPOINT_DESCRIPTOR HID_ReportINEndpoint;
} __attribute__((packed)) SWITCH_CONFIGURATION_DESCRIPTOR;

typedef struct {
    USB_CONFIGURATION_DESCRIPTOR Config;
    USB_INTERFACE_DESCRIPTOR Interface1;
    XBOX_ID_DESCRIPTOR Interface1ID;
    USB_ENDPOINT_DESCRIPTOR ReportINEndpoint11;
    USB_ENDPOINT_DESCRIPTOR ReportOUTEndpoint12;
    USB_INTERFACE_DESCRIPTOR Interface2;
    uint8_t UnkownDescriptor2[0x1B];
    USB_ENDPOINT_DESCRIPTOR ReportINEndpoint21;
    USB_ENDPOINT_DESCRIPTOR ReportOUTEndpoint22;
    USB_ENDPOINT_DESCRIPTOR ReportINEndpoint23;
    USB_ENDPOINT_DESCRIPTOR ReportOUTEndpoint24;
    USB_INTERFACE_DESCRIPTOR Interface3;
    uint8_t UnkownDescriptor3[0x09];
    USB_ENDPOINT_DESCRIPTOR ReportINEndpoint31;
    USB_INTERFACE_DESCRIPTOR Interface4;
    uint8_t UnkownDescriptor4[0x06];
} __attribute__((packed)) CONFIGURATION_XBOX_DESCRIPTOR;

typedef struct {
    USB_CONFIGURATION_DESCRIPTOR Config;
    USB_INTERFACE_DESCRIPTOR InterfaceHID;
    USB_HID_DESCRIPTOR HIDDescriptor;
    USB_ENDPOINT_DESCRIPTOR EndpointInHID;
    USB_ENDPOINT_DESCRIPTOR EndpointOutHID;
    USB_INTERFACE_DESCRIPTOR InterfaceConfig;
    USB_INTERFACE_DESCRIPTOR InterfaceExtra;
    USB_INTERFACE_DESCRIPTOR Interface4;
    uint8_t UnkownDescriptor4[0x06];
} __attribute__((packed)) HID_CONFIGURATION_DESCRIPTOR;

typedef struct {
    USB_CONFIGURATION_DESCRIPTOR Config;
    USB_INTERFACE_DESCRIPTOR Interface_AudioControl;
    AUDIO_INTERFACE_AC_DESCRIPTOR Audio_ControlInterface_SPC;
    USB_INTERFACE_DESCRIPTOR Interface_AudioStream;
    MIDI_AUDIOINTERFACE_AS_DESCRIPTOR Audio_StreamInterface_SPC;
    MIDI_INPUTJACK_DESCRIPTOR MIDI_In_Jack_Emb;
    MIDI_INPUTJACK_DESCRIPTOR MIDI_In_Jack_Ext;
    MIDI_OUTPUTJACK_DESCRIPTOR MIDI_Out_Jack_Emb;
    MIDI_OUTPUTJACK_DESCRIPTOR MIDI_Out_Jack_Ext;
    AUDIO_STREAMENDPOINT_DESCRIPTOR MIDI_In_Jack_Endpoint;
    MIDI_JACK_ENDPOINT_DESCRIPTOR MIDI_In_Jack_Endpoint_SPC;
    AUDIO_STREAMENDPOINT_DESCRIPTOR MIDI_Out_Jack_Endpoint;
    MIDI_JACK_ENDPOINT_DESCRIPTOR MIDI_Out_Jack_Endpoint_SPC;
    USB_INTERFACE_DESCRIPTOR InterfaceConfig;
} __attribute__((packed)) MIDI_CONFIGURATION_DESCRIPTOR;

extern const PROGMEM MIDI_CONFIGURATION_DESCRIPTOR MIDIConfigurationDescriptor;
extern const PROGMEM HID_CONFIGURATION_DESCRIPTOR HIDConfigurationDescriptor;
extern const PROGMEM CONFIGURATION_XBOX_DESCRIPTOR XBOXConfigurationDescriptor;
extern const PROGMEM CONFIGURATION_XBOX_PC_DESCRIPTOR XBOXConfigurationDescriptorPC;
extern const PROGMEM USB_DEVICE_DESCRIPTOR deviceDescriptor;
extern const PROGMEM OS_DESCRIPTOR OSDescriptorString;
extern const PROGMEM STRING_DESCRIPTOR* const descriptorStrings[3];
extern const PROGMEM STRING_DESCRIPTOR xboxString;
extern const PROGMEM STRING_DESCRIPTOR languageString;
extern const PROGMEM STRING_DESCRIPTOR manufacturerString;
extern const PROGMEM STRING_DESCRIPTOR productString;

#define VERSION_MAJOR 3
#define VERSION_MINOR 0
#define VERSION_REVISION 0
uint16_t controlRequest(const uint8_t requestType, const uint8_t request, const uint16_t wValue, const uint16_t wIndex, const uint16_t wLength, void* requestBuffer);
uint16_t descriptorRequest(const uint16_t wValue,
                           const uint16_t wIndex,
                           void* descriptorBuffer);
bool controlRequestValid(const uint8_t requestType, const uint8_t request, const uint16_t wValue, const uint16_t wIndex, const uint16_t wLength);