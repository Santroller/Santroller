#pragma once
#include <stdint.h>

#include "Usb.h"
#include "endpoints.h"
#include "midi.h"
#include <avr/pgmspace.h>
#include "wcid.h"
#include "xbox.h"
#include "string_descriptors.h"

#define USB_VERSION_BCD(Major, Minor, Revision) \
    (((Major & 0xFF) << 8) |                    \
     ((Minor & 0x0F) << 4) |                    \
     (Revision & 0x0F))

#define USB_CONFIG_POWER_MA(mA) ((mA) >> 1)

#define NO_DESCRIPTOR 0

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
    USB_INTERFACE_DESCRIPTOR InterfaceSecurity;
    uint8_t UnkownDescriptor4[0x06];
} __attribute__((packed)) XBOX_360_CONFIGURATION_DESCRIPTOR;

typedef struct {
    USB_CONFIGURATION_DESCRIPTOR Config;
    USB_INTERFACE_DESCRIPTOR InterfaceHID;
    USB_HID_DESCRIPTOR HIDDescriptor;
    USB_ENDPOINT_DESCRIPTOR EndpointInHID;
    USB_ENDPOINT_DESCRIPTOR EndpointOutHID;
    USB_INTERFACE_DESCRIPTOR InterfaceConfig;
    USB_INTERFACE_DESCRIPTOR InterfaceExtra;
    USB_INTERFACE_DESCRIPTOR InterfaceSecurity;
    uint8_t UnkownDescriptor4[0x06];
} __attribute__((packed)) UNIVERSAL_CONFIGURATION_DESCRIPTOR;


typedef struct {
    USB_CONFIGURATION_DESCRIPTOR Config;
    USB_INTERFACE_DESCRIPTOR Interface;
    USB_ENDPOINT_DESCRIPTOR EndpointIn;
    USB_ENDPOINT_DESCRIPTOR EndpointOut;
} __attribute__((packed)) XBOX_ONE_CONFIGURATION_DESCRIPTOR;
typedef struct {
    USB_CONFIGURATION_DESCRIPTOR Config;
    USB_INTERFACE_DESCRIPTOR InterfaceHID;
    USB_HID_DESCRIPTOR HIDDescriptor;
    USB_ENDPOINT_DESCRIPTOR EndpointInHID;
    USB_ENDPOINT_DESCRIPTOR EndpointOutHID;
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
extern const PROGMEM UNIVERSAL_CONFIGURATION_DESCRIPTOR UniversalConfigurationDescriptor;
extern const PROGMEM HID_CONFIGURATION_DESCRIPTOR HIDConfigurationDescriptor;
extern const PROGMEM XBOX_360_CONFIGURATION_DESCRIPTOR XBOX360ConfigurationDescriptor;
extern const PROGMEM USB_DEVICE_DESCRIPTOR deviceDescriptor;
extern const PROGMEM OS_DESCRIPTOR OSDescriptorString;
uint16_t controlRequest(const uint8_t requestType, const uint8_t request, const uint16_t wValue, const uint16_t wIndex, const uint16_t wLength, uint8_t* requestBuffer);
uint16_t descriptorRequest(const uint16_t wValue,
                           const uint16_t wIndex,
                           void* descriptorBuffer);
bool controlRequestValid(const uint8_t requestType, const uint8_t request, const uint16_t wValue, const uint16_t wIndex, const uint16_t wLength);