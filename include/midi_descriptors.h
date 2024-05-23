#pragma once
#include <stdint.h>

#define USB_SUBCLASS_CONTROL 0x01
#define USB_SUBCLASS_AUDIO_STREAMING 0x02
#define USB_SUBCLASS_MIDI_STREAMING 0x03
#define USB_PROTOCOL_CONTROL 0x00
#define USB_PROTOCOL_STREAMING 0x00

#define USB_DESCRIPTOR_AUDIO_CSINTERFACE 0x24
#define USB_DESCRIPTOR_AUDIO_CSENDPOINT 0x25

#define USB_AUDIO_SUBTYPE_CSINTERFACE_HEADER 0x01
#define USB_AUDIO_SUBTYPE_CSINTERFACE_INPUT_TERMINAL 0x02
#define USB_AUDIO_SUBTYPE_CSINTERFACE_OUTPUT_TERMINAL 0x03
#define USB_AUDIO_SUBTYPE_CSINTERFACE_MIXER 0x04
#define USB_AUDIO_SUBTYPE_CSINTERFACE_SELECTOR 0x05
#define USB_AUDIO_SUBTYPE_CSINTERFACE_FEATURE 0x06
#define USB_AUDIO_SUBTYPE_CSINTERFACE_PROCESSING 0x07
#define USB_AUDIO_SUBTYPE_CSINTERFACE_EXTENSION 0x08

#define USB_AUDIO_SUBTYPE_CSINTERFACE_GENERAL 0x01
#define USB_AUDIO_SUBTYPE_CSINTERFACE_FORMAT_TYPE 0x02
#define USB_AUDIO_SUBTYPE_CSINTERFACE_FORMAT_SPECIFIC 0x03

#define USB_AUDIO_SUBTYPE_CSENDPOINT_GENERAL 0x01

#define MIDI_JACK_TYPE_EMBEDDED 0x01
#define MIDI_JACK_TYPE_EXTERNAL 0x02

/** \brief Audio class-specific Interface Descriptor (USB-IF naming conventions).
 *
 *  Type define for an Audio class-specific interface descriptor. This follows a regular interface descriptor to
 *  supply extra information about the audio device's layout to the host. See the USB Audio specification for more
 *  details.
 *
 *  \see \ref TUSB_AUDIO_Descriptor_Interface_AC_t for the version of this type with non-standard LUFA specific
 *       element names.
 *
 *  \note Regardless of CPU architecture, these values should be stored as little endian.
 */
typedef struct
{
    uint8_t bLength;         /**< Size of the descriptor, in bytes. */
    uint8_t bDescriptorType; /**< Type of the descriptor, either a value in \ref TUSB_DescriptorTypes_t or a value
                              *   given by the specific class.
                              */

    uint8_t bDescriptorSubtype; /**< Sub type value used to distinguish between audio class-specific descriptors,
                                 *   a value from the \ref AUDIO_CS_INTERFACE_AS_SUB_TYPES enum.
                                 */

    uint16_t bcdADC;       /**< Binary coded decimal value, indicating the supported Audio Class specification version.
                            *
                            *   \see \ref USB_VERSION_BCD() utility macro.
                            */
    uint16_t wTotalLength; /**< Total length of the Audio class-specific descriptors, including this descriptor. */

    uint8_t bInCollection;     /**< Total number of Audio Streaming interfaces linked to this Audio Control interface (must be 1). */
    uint8_t bInterfaceNumbers; /**< Interface number of the associated Audio Streaming interface. */
} __attribute__((packed)) AUDIO_INTERFACE_AC_DESCRIPTOR;

/** \brief Audio class-specific Streaming Audio Interface Descriptor (USB-IF naming conventions).
 *
 *  Type define for an Audio class-specific streaming interface descriptor. This indicates to the host
 *  how audio streams within the device are formatted. See the USB Audio specification for more details.
 *
 *  \see \ref TUSB_AUDIO_Descriptor_Interface_AS_t for the version of this type with non-standard LUFA specific
 *       element names.
 *
 *  \note Regardless of CPU architecture, these values should be stored as little endian.
 */
typedef struct
{
    uint8_t bLength;         /**< Size of the descriptor, in bytes. */
    uint8_t bDescriptorType; /**< Type of the descriptor, either a value in \ref TUSB_DescriptorTypes_t or a value
                              *   given by the specific class.
                              */

    uint8_t bDescriptorSubtype; /**< Sub type value used to distinguish between audio class-specific descriptors,
                                 *   a value from the \ref AUDIO_CS_INTERFACE_AS_SUB_TYPES enum.
                                 */

    uint8_t bTerminalLink; /**< ID value of the output terminal this descriptor is describing. */

    uint8_t bDelay;      /**< Delay in frames resulting from the complete sample processing from input to output. */
    uint16_t wFormatTag; /**< Format of the audio stream, see Audio Device Formats specification. */
} __attribute__((packed)) AUDIO_INTERFACE_AS_DESCRIPTOR;

/** \brief Audio class-specific Streaming Endpoint Descriptor (USB-IF naming conventions).
 *
 *  Type define for an Audio class-specific endpoint descriptor. This contains a regular endpoint
 *  descriptor with a few Audio-class-specific extensions. See the USB Audio specification for more details.
 *
 *  \see \ref TUSB_AUDIO_Descriptor_StreamEndpoint_Std_t for the version of this type with non-standard LUFA specific
 *       element names.
 *
 *  \note Regardless of CPU architecture, these values should be stored as little endian.
 */
typedef struct
{
    uint8_t bLength;          /**< Size of the descriptor, in bytes. */
    uint8_t bDescriptorType;  /**< Type of the descriptor, either a value in \ref TUSB_DescriptorTypes_t or a
                               *   value given by the specific class.
                               */
    uint8_t bEndpointAddress; /**< Logical address of the endpoint within the device for the current
                               *   configuration, including direction mask.
                               */
    uint8_t bmAttributes;     /**< Endpoint attributes, comprised of a mask of the endpoint type (\c EP_TYPE_*)
                               *   and attributes (\c ENDPOINT_TATTR_*) masks.
                               */
    uint16_t wMaxPacketSize;  /**< Size of the endpoint bank, in bytes. This indicates the maximum packet size
                               *   that the endpoint can receive at a time.
                               */
    uint8_t bInterval;        /**< Polling interval in milliseconds for the endpoint if it is an INTERRUPT or
                               *   ISOCHRONOUS type.
                               */

    uint8_t bRefresh;      /**< Always set to zero for Audio class devices. */
    uint8_t bSynchAddress; /**< Endpoint address to send synchronization information to, if needed (zero otherwise). */
} __attribute__((packed)) AUDIO_STREAMENDPOINT_DESCRIPTOR;

/** \brief Audio class-specific Extended Endpoint Descriptor (USB-IF naming conventions).
 *
 *  Type define for an Audio class-specific extended endpoint descriptor. This contains extra information
 *  on the usage of endpoints used to stream audio in and out of the USB Audio device, and follows an Audio
 *  class-specific extended endpoint descriptor. See the USB Audio specification for more details.
 *
 *  \see \ref TUSB_AUDIO_Descriptor_StreamEndpoint_Spc_t for the version of this type with non-standard LUFA specific
 *       element names.
 *
 *  \note Regardless of CPU architecture, these values should be stored as little endian.
 */
typedef struct
{
    uint8_t bLength;         /**< Size of the descriptor, in bytes. */
    uint8_t bDescriptorType; /**< Type of the descriptor, either a value in \ref TUSB_DescriptorTypes_t or a value
                              *   given by the specific class.
                              */

    uint8_t bDescriptorSubtype; /**< Sub type value used to distinguish between audio class-specific descriptors,
                                 *   a value from the \ref AUDIO_CS_ENDPOINT_SUB_TYPES enum.
                                 */

    uint8_t bmAttributes; /**< Audio class-specific endpoint attributes, such as \ref AUDIO_EP_FULL_PACKETS_ONLY. */

    uint8_t bLockDelayUnits; /**< Units used for the LockDelay field, see Audio class specification. */
    uint16_t wLockDelay;     /**< Time required to internally lock endpoint's internal clock recovery circuitry. */
} __attribute__((packed)) AUDIO_STREAMENDPOINT_SPC_DESCRIPTOR;

// /* Macros: */
// /** \name MIDI Command Values
//  *  See http://www.midi.org/techspecs/midimessages.php for more information.
//  */
// /**@{*/
// /** MIDI command for System Exclusive (SysEx) single event that has one byte of data total. */
// #define MIDI_COMMAND_SYSEX_1BYTE MIDI_COMMAND_SYSEX_END_1BYTE

// /** MIDI command for System Exclusive (SysEx) single event that has two bytes of data total. */
// #define MIDI_COMMAND_SYSEX_2BYTE 0x20

// /** MIDI command for System Exclusive (SysEx) single event that has three bytes of data total. */
// #define MIDI_COMMAND_SYSEX_3BYTE 0x30

// /** MIDI command for System Exclusive (SysEx) stream event that has at least four bytes of data total. */
// #define MIDI_COMMAND_SYSEX_START_3BYTE 0x40

// /** MIDI command for System Exclusive (SysEx) stream event terminator with one remaining data byte. */
// #define MIDI_COMMAND_SYSEX_END_1BYTE 0x50

// /** MIDI command for System Exclusive (SysEx) stream event terminator with two remaining data bytes. */
// #define MIDI_COMMAND_SYSEX_END_2BYTE 0x60

// /** MIDI command for System Exclusive (SysEx) stream event terminator with three remaining data bytes. */
// #define MIDI_COMMAND_SYSEX_END_3BYTE 0x70

// /** MIDI command for a note off (deactivation) event. */
// #define MIDI_COMMAND_NOTE_OFF 0x80

// /** MIDI command for a note on (activation) event. */
// #define MIDI_COMMAND_NOTE_ON 0x90

// /** MIDI command for a note pressure change event. */
// #define MIDI_COMMAND_NOTE_PRESSURE 0xA0

// /** MIDI command for a control change event. */
// #define MIDI_COMMAND_CONTROL_CHANGE 0xB0

// /** MIDI command for a control change event. */
// #define MIDI_COMMAND_PROGRAM_CHANGE 0xC0

// /** MIDI command for a channel pressure change event. */
// #define MIDI_COMMAND_CHANNEL_PRESSURE 0xD0

// /** MIDI command for a pitch change event. */
// #define MIDI_COMMAND_PITCH_WHEEL_CHANGE 0xE0
// /**@}*/

// /** Standard key press velocity value used for all note events. */
// #define MIDI_STANDARD_VELOCITY 64

// /** Convenience macro. MIDI channels are numbered from 1-10 (natural numbers) however the logical channel
//  *  addresses are zero-indexed. This converts a natural MIDI channel number into the logical channel address.
//  *
//  *  \param[in] channel  MIDI channel number to address.
//  *
//  *  \return Constructed MIDI channel ID.
//  */
// #define MIDI_CHANNEL(channel) ((channel)-1)

// /** Constructs a MIDI event ID from a given MIDI command and a virtual MIDI cable index. This can then be
//  *  used to create and decode \ref MIDI_EventPacket_t MIDI event packets.
//  *
//  *  \param[in] virtualcable  Index of the virtual MIDI cable the event relates to
//  *  \param[in] command       MIDI command to send through the virtual MIDI cable
//  *
//  *  \return Constructed MIDI event ID.
//  */
// #define MIDI_EVENT(virtualcable, command) (((virtualcable) << 4) | ((command) >> 4))

/** \brief MIDI class-specific Streaming Interface Descriptor (USB-IF naming conventions).
 *
 *  Type define for an Audio class-specific MIDI streaming interface descriptor. This indicates to the host
 *  how MIDI the specification compliance of the device and the total length of the Audio class-specific descriptors.
 *  See the USB Audio specification for more details.
 *
 *  \see \ref TUSB_MIDI_Descriptor_AudioInterface_AS_t for the version of this type with non-standard LUFA specific
 *       element names.
 *
 *  \note Regardless of CPU architecture, these values should be stored as little endian.
 */
typedef struct
{
    uint8_t bLength;         /**< Size of the descriptor, in bytes. */
    uint8_t bDescriptorType; /**< Type of the descriptor, either a value in \ref TUSB_DescriptorTypes_t or a value
                              *   given by the specific class.
                              */

    uint8_t bDescriptorSubtype; /**< Sub type value used to distinguish between audio class-specific descriptors. */

    uint16_t bcdMSC;       /**< Binary coded decimal value, indicating the supported MIDI Class specification version.
                            *
                            *   \see \ref USB_VERSION_BCD() utility macro.
                            */
    uint16_t wTotalLength; /**< Total length of the Audio class-specific descriptors, including this descriptor. */
} __attribute__((packed)) MIDI_AUDIOINTERFACE_AS_DESCRIPTOR;

/** \brief MIDI class-specific Input Jack Descriptor (USB-IF naming conventions).
 *
 *  Type define for an Audio class-specific MIDI IN jack. This gives information to the host on a MIDI input, either
 *  a physical input jack, or a logical jack (receiving input data internally, or from the host via an endpoint).
 *
 *  \see \ref TUSB_MIDI_Descriptor_InputJack_t for the version of this type with non-standard LUFA specific
 *       element names.
 *
 *  \note Regardless of CPU architecture, these values should be stored as little endian.
 */
typedef struct
{
    uint8_t bLength;         /**< Size of the descriptor, in bytes. */
    uint8_t bDescriptorType; /**< Type of the descriptor, either a value in \ref TUSB_DescriptorTypes_t or a value
                              *   given by the specific class.
                              */

    uint8_t bDescriptorSubtype; /**< Sub type value used to distinguish between audio class-specific descriptors. */

    uint8_t bJackType; /**< Type of jack, one of the \c JACKTYPE_* mask values. */
    uint8_t bJackID;   /**< ID value of this jack - must be a unique value within the device. */

    uint8_t iJack; /**< Index of a string descriptor describing this descriptor within the device. */
} __attribute__((packed)) MIDI_INPUTJACK_DESCRIPTOR;

/** \brief MIDI class-specific Output Jack Descriptor (USB-IF naming conventions).
 *
 *  Type define for an Audio class-specific MIDI OUT jack. This gives information to the host on a MIDI output, either
 *  a physical output jack, or a logical jack (sending output data internally, or to the host via an endpoint).
 *
 *  \see \ref TUSB_MIDI_Descriptor_OutputJack_t for the version of this type with non-standard LUFA specific
 *       element names.
 *
 *  \note Regardless of CPU architecture, these values should be stored as little endian.
 */
typedef struct
{
    uint8_t bLength;         /**< Size of the descriptor, in bytes. */
    uint8_t bDescriptorType; /**< Type of the descriptor, either a value in \ref TUSB_DescriptorTypes_t or a value
                              *   given by the specific class.
                              */

    uint8_t bDescriptorSubtype; /**< Sub type value used to distinguish between audio class-specific descriptors. */

    uint8_t bJackType; /**< Type of jack, one of the \c JACKTYPE_* mask values. */
    uint8_t bJackID;   /**< ID value of this jack - must be a unique value within the device. */

    uint8_t bNrInputPins;   /**< Number of output channels within the jack, either physical or logical. */
    uint8_t baSourceID[1];  /**< ID of each output pin's source data jack. */
    uint8_t baSourcePin[1]; /**< Pin number in the input jack of each output pin's source data. */

    uint8_t iJack; /**< Index of a string descriptor describing this descriptor within the device. */
} __attribute__((packed)) MIDI_OUTPUTJACK_DESCRIPTOR;

/** \brief Audio class-specific Jack Endpoint Descriptor (USB-IF naming conventions).
 *
 *  Type define for an Audio class-specific extended MIDI jack endpoint descriptor. This contains extra information
 *  on the usage of MIDI endpoints used to stream MIDI events in and out of the USB Audio device, and follows an Audio
 *  class-specific extended MIDI endpoint descriptor. See the USB Audio specification for more details.
 *
 *  \see \ref TUSB_MIDI_Descriptor_Jack_Endpoint_t for the version of this type with non-standard LUFA specific
 *       element names.
 *
 *  \note Regardless of CPU architecture, these values should be stored as little endian.
 */
typedef struct
{
    uint8_t bLength;         /**< Size of the descriptor, in bytes. */
    uint8_t bDescriptorType; /**< Type of the descriptor, either a value in \ref TUSB_DescriptorTypes_t or a value
                              *   given by the specific class.
                              */

    uint8_t bDescriptorSubtype; /**< Sub type value used to distinguish between audio class-specific descriptors. */

    uint8_t bNumEmbMIDIJack; /**< Total number of jacks inside this endpoint. */
    uint8_t bAssocJackID[1]; /**< IDs of each jack inside the endpoint. */
} __attribute__((packed)) MIDI_JACK_ENDPOINT_DESCRIPTOR;

/** \brief MIDI Class Driver Event Packet.
 *
 *  Type define for a USB MIDI event packet, used to encapsulate sent and received MIDI messages from a USB MIDI interface.
 *
 *  \note Regardless of CPU architecture, these values should be stored as little endian.
 */
typedef struct
{
    uint8_t Event; /**< MIDI event type, constructed with the \ref MIDI_EVENT() macro. */

    uint8_t Data1; /**< First byte of data in the MIDI event. */
    uint8_t Data2; /**< Second byte of data in the MIDI event. */
    uint8_t Data3; /**< Third byte of data in the MIDI event. */
} __attribute__((packed)) MIDI_EVENT_PACKET;

typedef struct
{
    uint8_t midiVelocities[128];
    int16_t midiPitchWheel;
    uint8_t midiModWheel;
    uint8_t midiSustainPedal;
} __attribute__((packed)) Midi_Data_t;