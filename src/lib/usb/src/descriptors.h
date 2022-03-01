#include <stdint.h>
#include "Usb.h"
#include "wcid.h"
#include "xbox.h"
#include "midi.h"
#include "usb_macros.h"

#define ENDPOINT_SIZE 64
#define VENDOR_EPSIZE 64

#define ARDWIINO_VID 0x1209
#define ARDWIINO_PID 0x2882
#define HORI_VID 0x0f0d
#define HORI_POKKEN_TOURNAMENT_DX_PRO_PAD_PID 0x0092
#define SONY_VID 0x12ba
#define PS3_GH_GUITAR_PID 0x0100
#define PS3_GH_DRUM_PID 0x0120
#define PS3_RB_GUITAR_PID 0x0200
#define PS3_RB_DRUM_PID 0x0210
#define PS3_DJ_TURNTABLE_PID 0x0140
#define PS3WIIU_GHLIVE_DONGLE_PID 0x074b
#define WII_RB_VID 0x1bad
#define WII_RB_GUITAR_PID 0x0004
#define WII_RB_DRUM_PID 0x074b
#define NINDENDO_VID 0x057E
#define PRO_CONTROLLER_PID 0x2009

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

// By using these endpoints, we can double buffer!
enum endpoints_t {
    DEVICE_EPADDR_IN = ENDPOINT_IN | 1,
    DEVICE_EPADDR_OUT = ENDPOINT_OUT | 2
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
        0, /**< MIDI Control Stream interface descriptor ID */
    INTERFACE_ID_AudioStream =
        2, /**< MIDI Audio Stream interface descriptor ID */
};

typedef struct
{
    uint8_t bLength; // Length of this descriptor.
    uint8_t bDescriptorType; // CONFIGURATION descriptor type (USB_DESCRIPTOR_CONFIGURATION).
    uint16_t UnicodeString[];       /**< String data, as unicode characters (alternatively,
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
extern const PROGMEM STRING_DESCRIPTOR *const descriptorStrings[3];
extern const PROGMEM STRING_DESCRIPTOR xboxString;
extern const PROGMEM STRING_DESCRIPTOR languageString;
extern const PROGMEM STRING_DESCRIPTOR manufacturerString;
extern const PROGMEM STRING_DESCRIPTOR productString;

#define VERSION_MAJOR 3
#define VERSION_MINOR 0
#define VERSION_REVISION 0