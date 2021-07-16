#pragma once
#include <stdbool.h>
#include <stdint.h>

#include "usb/hid_descriptors.h"
#include "usb/midi_descriptors.h"
#include "usb/std_descriptors.h"
#include "usb/wcid_descriptors.h"

#define ENDPOINT_SIZE 64
#define XINPUT_EPSIZE_IN 0x14
#define XINPUT_EPSIZE_OUT 0x08
#define HID_EPSIZE_IN 32
#define HID_EPSIZE_OUT 0x08
#define VENDOR_EPSIZE 64

#define ARDWIINO_VID 0x1209
#define ARDWIINO_PID 0x2882
#define HORI_VID 0x0f0d
#define HORI_POKKEN_TOURNAMENT_DX_PRO_PAD_PID 0x0092
#define SONY_VID 0x12ba
#define PS3_GH_GUITAR_VID 0x0100
#define PS3_GH_DRUM_VID 0x0120
#define PS3_RB_GUITAR_VID 0x0200
#define PS3_RB_DRUM_VID 0x0210
#define WII_RB_VID 0x1bad
#define WII_RB_GUITAR 0x0004
#define WII_RB_DRUM 0x074b

#define SIMULTANEOUS_KEYS 6

#define Buttons 4
#define MinAxisVal -127
#define MaxAxisVal 127
#define MinPhysicalVal -127
#define MaxPhysicalVal 128
#define AbsoluteCoords false

enum endpoint_dir_t {
    ENDPOINT_OUT = 0x00,
    ENDPOINT_IN = 0x80
};

enum descriptors_t {
    DESC_EXTENDED_COMPATIBLE_ID_DESCRIPTOR = 0x0004,
    DESC_EXTENDED_PROPERTIES_DESCRIPTOR = 0x0005,
};

enum requests_t {
    REQ_GET_OS_FEATURE_DESCRIPTOR = 0x20
};

enum endpoints_t {
    DEVICE_EPADDR_IN = ENDPOINT_IN | 1,
    DEVICE_EPADDR_OUT = ENDPOINT_OUT | 1,
};

enum hid_reports_t {
    REPORT_ID_MOUSE = 1,
    REPORT_ID_KBD,
};

enum interfaces_t {
    INTERFACE_ID_Device = 0, /**< XInput interface descriptor ID */
    INTERFACE_ID_Config = 1, /**< Config interface descriptor ID */
    INTERFACE_ID_Padding = 2,
    INTERFACE_ID_XBOX_Security = 3,
    INTERFACE_ID_ControlStream =
        1, /**< MIDI Control Stream interface descriptor ID */
    INTERFACE_ID_AudioStream =
        2, /**< MIDI Audio Stream interface descriptor ID */
};

typedef struct {
    TUSB_Descriptor_Header_t Header;
    uint8_t reserved[2];
    uint8_t subtype;
    uint8_t reserved2;
    uint8_t bEndpointAddressIn;
    uint8_t bMaxDataSizeIn;
    uint8_t reserved3[5];
    uint8_t bEndpointAddressOut;
    uint8_t bMaxDataSizeOut;
    uint8_t reserved4[2];
} XBOX_ID_Descriptor_t;

typedef struct {
    TUSB_Descriptor_Configuration_Header_t Config;
    TUSB_Descriptor_Interface_t Interface1;
    XBOX_ID_Descriptor_t Interface1ID;
    TUSB_Descriptor_Endpoint_t ReportINEndpoint11;
    TUSB_Descriptor_Endpoint_t ReportOUTEndpoint12;
    TUSB_Descriptor_Interface_t Interface4;
    uint8_t UnkownDescriptor4[0x06];
    TUSB_Descriptor_Interface_t InterfaceConfig;
    TUSB_Descriptor_Interface_t InterfaceExtra;
} TUSB_Descriptor_Configuration_XBOX_t;

typedef struct {
    TUSB_Descriptor_Configuration_Header_t Config;
    TUSB_Descriptor_Interface_t InterfaceHID;
    TUSB_THID_Descriptor_THID_t HIDDescriptor;
    TUSB_Descriptor_Endpoint_t EndpointInHID;
    TUSB_Descriptor_Endpoint_t EndpointOutHID;
    TUSB_Descriptor_Interface_t InterfaceConfig;
} TUSB_Descriptor_HID_Configuration_t;

typedef struct {
    TUSB_Descriptor_Configuration_Header_t Config;
    TUSB_Descriptor_Interface_t Interface_AudioControl;
    TUSB_TAUDIO_Descriptor_Interface_AC_t Audio_ControlInterface_SPC;
    TUSB_Descriptor_Interface_t Interface_AudioStream;
    TUSB_MIDI_Descriptor_AudioInterface_AS_t Audio_StreamInterface_SPC;
    TUSB_MIDI_Descriptor_InputJack_t MIDI_In_Jack_Emb;
    TUSB_MIDI_Descriptor_InputJack_t MIDI_In_Jack_Ext;
    TUSB_MIDI_Descriptor_OutputJack_t MIDI_Out_Jack_Emb;
    TUSB_MIDI_Descriptor_OutputJack_t MIDI_Out_Jack_Ext;
    TUSB_TAUDIO_Descriptor_StreamEndpoint_Std_t MIDI_In_Jack_Endpoint;
    TUSB_MIDI_Descriptor_Jack_Endpoint_t MIDI_In_Jack_Endpoint_SPC;
    TUSB_TAUDIO_Descriptor_StreamEndpoint_Std_t MIDI_Out_Jack_Endpoint;
    TUSB_MIDI_Descriptor_Jack_Endpoint_t MIDI_Out_Jack_Endpoint_SPC;
    TUSB_Descriptor_Interface_t InterfaceConfig;
} TUSB_Descriptor_MIDI_Configuration_t;

extern TUSB_Descriptor_MIDI_Configuration_t MIDIConfigurationDescriptor;
extern TUSB_Descriptor_HID_Configuration_t HIDConfigurationDescriptor;
extern TUSB_Descriptor_Configuration_XBOX_t XBOXConfigurationDescriptor;
extern TUSB_Descriptor_Device_t deviceDescriptor;
extern const TUSB_OSDescriptor_t OSDescriptorString;
extern const TUSB_Descriptor_String_t *const descriptorStrings[3];
extern const TUSB_Descriptor_String_t xboxString;
