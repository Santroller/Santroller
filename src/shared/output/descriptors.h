#pragma once
#ifdef __AVR__
#  define AVR_CONST const PROGMEM
#else
#  define AVR_CONST
#endif
#define WCHAR wchar_t
/* Includes: (don't import everything on the 328p)*/
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega2560__) ||              \
    !defined(__AVR__)
// Pull in enough information from LUFA in order to be able to compile the
// descriptors.
#  define __INCLUDE_FROM_USB_DRIVER
#  define __INCLUDE_FROM_HID_DRIVER
#  define __INCLUDE_FROM_CDC_DRIVER
#  define __INCLUDE_FROM_MIDI_DRIVER
#  define USB_CAN_BE_DEVICE
#  include "LUFAConfig.h"
#  include "USBController.h"
#  include <LUFA/Drivers/USB/Class/Common/CDCClassCommon.h>
#  include <LUFA/Drivers/USB/Class/Common/HIDClassCommon.h>
#  include <LUFA/Drivers/USB/Class/Common/MIDIClassCommon.h>
#  include <LUFA/Drivers/USB/Core/StdDescriptors.h>

void Endpoint_Write_Control_Stream_LE(const void *const Buffer,
                                      uint16_t Length);
#else
#  include <LUFA/Drivers/USB/USB.h>
extern uint8_t deviceType;
#endif

#include "controller/controller.h"
#include "serial_commands.h"
#include "util/util.h"
// #include <avr/pgmspace.h>
#include <stdint.h>

// Device Request for WCID data. Note that this is the same as
// CDC_REQ_SetLineEncoding, and only the bmRequestType differs.
#define REQ_GetOSFeatureDescriptor 0x20

/** Descriptor index for a Microsoft Proprietary Extended Device Compatibility
 * descriptor. */
#define EXTENDED_COMPAT_ID_DESCRIPTOR 0x0004

/** Descriptor index for a Microsoft Proprietary Extended Device Properties OS
 * Feature Descriptor. */
#define EXTENDED_PROPERTIES_DESCRIPTOR 0x0005

#define HID_EPSIZE 32
/** Endpoint address of the DEVICE IN endpoint. */
#define XINPUT_EPADDR_IN (ENDPOINT_DIR_IN | 1)
/** Endpoint address of the DEVICE OUT endpoint. */
#define XINPUT_2_EPADDR_IN (ENDPOINT_DIR_IN | 2)
#define XINPUT_3_EPADDR_IN (ENDPOINT_DIR_IN | 3)
#define XINPUT_4_EPADDR_IN (ENDPOINT_DIR_IN | 4)
#define HID_EPADDR_IN (ENDPOINT_DIR_IN | 2)
/** Endpoint address of the DEVICE IN endpoint. */
#define MIDI_EPADDR_IN (ENDPOINT_DIR_IN | 3)
/** Endpoint address of the DEVICE IN endpoint. */
// We don't actually utilise the next descriptors, and since the UNO limits
// us to 4 endpoints, putting them last ensures that they are the unusable
// endpoints.
/** Endpoint address of the DEVICE OUT endpoint.*/
#define XINPUT_EPADDR_OUT (ENDPOINT_DIR_OUT | 7)
/** Endpoint address of the DEVICE OUT endpoint.  */
#define MIDI_EPADDR_OUT (ENDPOINT_DIR_OUT | 8)
#define XINPUT_2_EPADDR_OUT (ENDPOINT_DIR_OUT | 9)
#define XINPUT_3_EPADDR_OUT (ENDPOINT_DIR_OUT | 10)
#define XINPUT_4_EPADDR_OUT (ENDPOINT_DIR_OUT | 11)
typedef struct {
  USB_Descriptor_Header_t Header;
  uint8_t reserved[2];
  uint8_t subtype;
  uint8_t reserved2;
  uint8_t bEndpointAddressIn;
  uint8_t bMaxDataSizeIn;
  uint8_t reserved3[5];
  uint8_t bEndpointAddressOut;
  uint8_t bMaxDataSizeOut;
  uint8_t reserved4[2];

} USB_HID_XBOX_Descriptor_HID_t;

typedef struct {
  USB_Descriptor_Configuration_Header_t Config;
  USB_Descriptor_Interface_t InterfaceXInput;
  USB_HID_XBOX_Descriptor_HID_t XInputReserved;
  USB_Descriptor_Endpoint_t EndpointInXInput;
  USB_Descriptor_Endpoint_t EndpointOutXInput;
#ifdef MULTI_ADAPTOR
  USB_Descriptor_Interface_t InterfaceXInput2;
  USB_HID_XBOX_Descriptor_HID_t XInputReserved2;
  USB_Descriptor_Endpoint_t EndpointInXInput2;
  USB_Descriptor_Endpoint_t EndpointOutXInput2;
  USB_Descriptor_Interface_t InterfaceXInput3;
  USB_HID_XBOX_Descriptor_HID_t XInputReserved3;
  USB_Descriptor_Endpoint_t EndpointInXInput3;
  USB_Descriptor_Endpoint_t EndpointOutXInput3;
  USB_Descriptor_Interface_t InterfaceXInput4;
  USB_HID_XBOX_Descriptor_HID_t XInputReserved4;
  USB_Descriptor_Endpoint_t EndpointInXInput4;
  USB_Descriptor_Endpoint_t EndpointOutXInput4;
#endif
  USB_Descriptor_Interface_t InterfaceHID;
  USB_HID_Descriptor_HID_t HIDDescriptor;
  USB_Descriptor_Endpoint_t EndpointInHID;
  USB_Descriptor_Interface_t InterfaceConfig;
#ifndef MULTI_ADAPTOR
  USB_Descriptor_Interface_t Interface_AudioControl;
  USB_Audio_Descriptor_Interface_AC_t Audio_ControlInterface_SPC;
  USB_Descriptor_Interface_t Interface_AudioStream;
  USB_MIDI_Descriptor_AudioInterface_AS_t Audio_StreamInterface_SPC;
  USB_MIDI_Descriptor_InputJack_t MIDI_In_Jack_Emb;
  USB_MIDI_Descriptor_InputJack_t MIDI_In_Jack_Ext;
  USB_MIDI_Descriptor_OutputJack_t MIDI_Out_Jack_Emb;
  USB_MIDI_Descriptor_OutputJack_t MIDI_Out_Jack_Ext;
  USB_Audio_Descriptor_StreamEndpoint_Std_t MIDI_In_Jack_Endpoint;
  USB_MIDI_Descriptor_Jack_Endpoint_t MIDI_In_Jack_Endpoint_SPC;
  USB_Audio_Descriptor_StreamEndpoint_Std_t MIDI_Out_Jack_Endpoint;
  USB_MIDI_Descriptor_Jack_Endpoint_t MIDI_Out_Jack_Endpoint_SPC;
#endif
} USB_Descriptor_Configuration_t;

typedef struct {
  USB_Descriptor_Header_t Header;
  uint16_t Signature[7];
  uint8_t VendorCode;
  uint8_t Reserved;
} ATTR_PACKED USB_OSDescriptor_t;

typedef struct {
  uint8_t FirstInterfaceNumber;
  uint8_t Reserved;
  uint8_t CompatibleID[8];
  uint8_t SubCompatibleID[8];
  uint8_t Reserved2[6];
} ATTR_PACKED USB_OSCompatibleSection_t;

typedef struct {
  uint32_t TotalLength;
  uint16_t Version;
  uint16_t Index;
  uint8_t TotalSections;
  uint8_t Reserved[7];
  USB_OSCompatibleSection_t CompatID;
  USB_OSCompatibleSection_t CompatID2;
  USB_OSCompatibleSection_t CompatID3;
  USB_OSCompatibleSection_t CompatID4;
} ATTR_PACKED USB_OSCompatibleIDDescriptor_4_t;
typedef struct {
  uint32_t TotalLength;
  uint16_t Version;
  uint16_t Index;
  uint8_t TotalSections;
  uint8_t Reserved[7];
  USB_OSCompatibleSection_t CompatID;
  USB_OSCompatibleSection_t CompatID2;
} ATTR_PACKED USB_OSCompatibleIDDescriptor_t;
typedef struct {
  uint32_t PropertyDataType;
  uint16_t PropertyNameLength;
  uint16_t PropertyName[20];
  uint32_t PropertyDataLength;
  uint16_t PropertyData[39];
} ATTR_PACKED USB_OSExtendedSection_t;
typedef struct {
  uint32_t TotalLength;
  uint16_t Version;
  uint16_t Index;
  uint16_t TotalSections;
  uint32_t SectionSize;
  USB_OSExtendedSection_t ExtendedID;
} ATTR_PACKED USB_OSExtendedCompatibleIDDescriptor_t;
uint16_t USB_GetOSFeatureDescriptor(const uint8_t InterfaceNumber,
                                    const uint8_t wIndex,
                                    const uint8_t Recipient,
                                    const void **const DescriptorAddress)
    ATTR_WARN_UNUSED_RESULT ATTR_NON_NULL_PTR_ARG(4);

/* Function Prototypes: */
uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                    const uint16_t wIndex,
                                    const void **const DescriptorAddress)
    ATTR_WARN_UNUSED_RESULT ATTR_NON_NULL_PTR_ARG(3);
typedef enum {
  HID_USAGE_POINTER = 0x01,
  HID_USAGE_MOUSE = 0x02,
  HID_USAGE_GAMEPAD = 0x05,
  HID_USAGE_KEYBOARD = 0x06,
  HID_USAGE_HAT_SWITCH = 0x39,
  HID_USAGE_X = 0x30,
  HID_USAGE_Y,
  HID_USAGE_Z,
  HID_USAGE_Rx,
  HID_USAGE_Ry,
  HID_USAGE_Rz,
  HID_USAGE_Slider,
  HID_USAGE_Dial,
  HID_USAGE_Wheel,
  HID_USAGE_COUNTED_BUFFER = 0x3A,
} HID_Usage;
typedef enum {
  HID_USAGE_PAGE_GENERIC_DESKTOP = 0x01,
} HID_Usage_Page;

typedef enum { HID_UNIT_NONE, HID_UNIT_DEGREES = 0x14 } HID_Unit;
#ifdef __AVR__

typedef enum {
  HID_COLLECTION_PHYSICAL = 0x00,
  HID_COLLECTION_APPLICATION = 0x01,
  HID_COLLECTION_LOGICAL = 0x02,
} HID_Collection;
typedef enum { HID_USAGE_CONSUMER_AC_PAN = 0x0238 } HID_Usage2;
typedef enum {
  HID_USAGE_PAGE_KEYBOARD = 0x07,
  HID_USAGE_PAGE_LED = 0x08,
  HID_USAGE_PAGE_BUTTON = 0x09,
  HID_USAGE_PAGE_CONSUMER = 0x0c,
} HID_Usage_Page2;

#endif
typedef enum {
  REPORT_ID_XINPUT,
  REPORT_ID_XINPUT_2,
  REPORT_ID_XINPUT_3,
  REPORT_ID_XINPUT_4,
  REPORT_ID_GAMEPAD,
  REPORT_ID_MOUSE,
  REPORT_ID_KBD,
  REPORT_ID_MIDI,
  REPORT_ID_CONTROL
} HID_Report;

typedef union {
  USB_Descriptor_Configuration_t t;
  USB_Descriptor_Device_t d;
} Descriptor;

extern AVR_CONST USB_Descriptor_String_t AVR_CONST languageString;
extern AVR_CONST USB_Descriptor_String_t AVR_CONST manufacturerString;
extern AVR_CONST USB_Descriptor_String_t AVR_CONST productString;
extern const USB_Descriptor_String_t *AVR_CONST descriptorStrings[];
extern AVR_CONST USB_OSDescriptor_t OSDescriptorString;
extern AVR_CONST USB_Descriptor_HIDReport_Datatype_t
    ps3_report_descriptor[137];
extern AVR_CONST USB_Descriptor_HIDReport_Datatype_t
    kbd_report_descriptor[137];
extern AVR_CONST USB_Descriptor_Device_t deviceDescriptor;
extern AVR_CONST USB_Descriptor_Configuration_t ConfigurationDescriptor;
extern AVR_CONST uint16_t vid[];
extern AVR_CONST uint16_t pid[];