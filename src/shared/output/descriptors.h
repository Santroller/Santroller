#pragma once

/* Includes: (don't import everything on the 328p)*/
#ifdef __AVR_ATmega328P__
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
#else
#  include <LUFA/Drivers/USB/USB.h>
#endif

#include "util/util.h"
#include <avr/pgmspace.h>
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
#define HID_REPORTSIZE 32
/** Endpoint address of the DEVICE IN endpoint. */
#define DEVICE_EPADDR_IN (ENDPOINT_DIR_IN | 1)
/** Endpoint address of the CDC device-to-host notification IN endpoint. */
#define CDC_NOTIFICATION_EPADDR (ENDPOINT_DIR_IN | 2)

/** Endpoint address of the CDC device-to-host data IN endpoint. */
#define CDC_TX_EPADDR (ENDPOINT_DIR_IN | 3)

/** Endpoint address of the CDC host-to-device data OUT endpoint. */
#define CDC_RX_EPADDR (ENDPOINT_DIR_OUT | 4)
/** Endpoint address of the DEVICE OUT endpoint. */
#define DEVICE_EPADDR_OUT (ENDPOINT_DIR_OUT | 2)

/** Size in bytes of the CDC device-to-host notification IN endpoint. */
#define CDC_NOTIFICATION_EPSIZE 8

/** Size in bytes of the CDC data IN and OUT endpoints. */
#define CDC_TX_EPSIZE 64
#define CDC_TX_BANK_SIZE 2
#define CDC_RX_EPSIZE 32
#define CDC_RX_BANK_SIZE 1
/** Enum for the device interface descriptor IDs within the device. Each
 * interface descriptor should have a unique ID index associated with it, which
 * can be used to refer to the interface from other descriptors.
 */
enum InterfaceDescriptors_t {
  INTERFACE_ID_HID = 0,     /**< HID interface descriptor ID */
  INTERFACE_ID_Unused = 1,  /**< Unused HID interface descriptor ID */
  INTERFACE_ID_CDC_CCI = 2, /**< CDC CCI interface descriptor ID */
  INTERFACE_ID_CDC_DCI = 3, /**< CDC DCI interface descriptor ID */
  INTERFACE_ID_ControlStream =
      0, /**< MIDI Control Stream interface descriptor ID */
  INTERFACE_ID_AudioStream =
      1, /**< MIDI Audio Stream interface descriptor ID */
};
typedef struct {
  USB_Descriptor_Header_t Header;
  uint8_t reserved[2];
  uint8_t subtype;
  uint8_t reserved2[15];
} USB_HID_XBOX_Descriptor_HID_t;
typedef struct {
  USB_Descriptor_Interface_Association_t CDC_IAD;
  USB_Descriptor_Interface_t CDC_CCI_Interface;
  USB_CDC_Descriptor_FunctionalHeader_t CDC_Functional_Header;
  USB_CDC_Descriptor_FunctionalACM_t CDC_Functional_ACM;
  USB_CDC_Descriptor_FunctionalUnion_t CDC_Functional_Union;
  USB_Descriptor_Endpoint_t CDC_NotificationEndpoint;
  USB_Descriptor_Interface_t CDC_DCI_Interface;
  USB_Descriptor_Endpoint_t CDC_DataOutEndpoint;
  USB_Descriptor_Endpoint_t CDC_DataInEndpoint;
  USB_Descriptor_Interface_t Interface_AudioControl;
  USB_Audio_Descriptor_Interface_AC_t Audio_ControlInterface_SPC;
} CDC_t;
typedef struct {
  USB_Descriptor_Interface_t Interface0;
  USB_HID_Descriptor_HID_t HIDDescriptor;
  USB_HID_XBOX_Descriptor_HID_t XInputReserved;
  USB_MIDI_Descriptor_AudioInterface_AS_t Audio_StreamInterface_SPC;
  USB_MIDI_Descriptor_InputJack_t MIDI_In_Jack_Emb;
  USB_MIDI_Descriptor_InputJack_t MIDI_In_Jack_Ext;
  USB_MIDI_Descriptor_OutputJack_t MIDI_Out_Jack_Emb;
  USB_MIDI_Descriptor_OutputJack_t MIDI_Out_Jack_Ext;
  USB_Audio_Descriptor_StreamEndpoint_Std_t DataInEndpoint0;
  USB_MIDI_Descriptor_Jack_Endpoint_t MIDI_Out_Jack_Endpoint_SPC;
  USB_Audio_Descriptor_StreamEndpoint_Std_t DataOutEndpoint0;
  USB_MIDI_Descriptor_Jack_Endpoint_t MIDI_In_Jack_Endpoint_SPC;
} OTHER_t;
typedef struct {
  USB_Descriptor_Configuration_Header_t Config;
  OTHER_t other;
  CDC_t cdc;
} USB_Descriptor_Configuration_t;
typedef struct {
  USB_Descriptor_Configuration_Header_t Config;
  CDC_t cdc;
  OTHER_t other;
} USB_Descriptor_Configuration_midi_t;

typedef struct {
  USB_Descriptor_Header_t Header;
  WCHAR Signature[7];
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
} ATTR_PACKED USB_OSCompatibleIDDescriptor_t;
extern uint8_t deviceType;
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

#define HID_DESCRIPTOR_MOUSE_SCROLL(MinAxisVal, MaxAxisVal, MinPhysicalVal,    \
                                    MaxPhysicalVal, Buttons, AbsoluteCoords)   \
  HID_RI_USAGE_PAGE(8, 0x01), HID_RI_USAGE(8, 0x02),                           \
      HID_RI_COLLECTION(8, 0x01), HID_RI_USAGE(8, 0x01),                       \
      HID_RI_COLLECTION(8, 0x00), HID_RI_USAGE_PAGE(8, 0x09),                  \
      HID_RI_USAGE_MINIMUM(8, 0x01), HID_RI_USAGE_MAXIMUM(8, Buttons),         \
      HID_RI_LOGICAL_MINIMUM(8, 0x00), HID_RI_LOGICAL_MAXIMUM(8, 0x01),        \
      HID_RI_REPORT_COUNT(8, Buttons), HID_RI_REPORT_SIZE(8, 0x01),            \
      HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),     \
      HID_RI_REPORT_COUNT(8, 0x01),                                            \
      HID_RI_REPORT_SIZE(8, (Buttons % 8) ? (8 - (Buttons % 8)) : 0),          \
      HID_RI_INPUT(8, HID_IOF_CONSTANT), HID_RI_USAGE_PAGE(8, 0x01),           \
      HID_RI_USAGE(8, 0x30), HID_RI_USAGE(8, 0x31),                            \
      HID_RI_LOGICAL_MINIMUM(16, MinAxisVal),                                  \
      HID_RI_LOGICAL_MAXIMUM(16, MaxAxisVal),                                  \
      HID_RI_PHYSICAL_MINIMUM(16, MinPhysicalVal),                             \
      HID_RI_PHYSICAL_MAXIMUM(16, MaxPhysicalVal),                             \
      HID_RI_REPORT_COUNT(8, 0x02),                                            \
      HID_RI_REPORT_SIZE(                                                      \
          8, (((MinAxisVal >= -128) && (MaxAxisVal <= 127)) ? 8 : 16)),        \
      HID_RI_INPUT(                                                            \
          8, HID_IOF_DATA | HID_IOF_VARIABLE |                                 \
                 (AbsoluteCoords ? HID_IOF_ABSOLUTE : HID_IOF_RELATIVE)),      \
      0x09, 0x38, 0x15, 0x81, 0x25, 0x7f, 0x35, 0x00, 0x45, 0x00, 0x75, 0x08,  \
      0x81, 0x06, HID_RI_END_COLLECTION(0), HID_RI_END_COLLECTION(0)
