#pragma once

/* Includes: */
#include <LUFA/Drivers/USB/USB.h>

#include "../../../config/config.h"
#include <avr/pgmspace.h>
/* Enums */
enum registry_props_type {
  REG_SZ = 1,
  REG_EXPAND_SZ,
  REG_BINARY,
  REG_DWORD_LITTLE_ENDIAN,
  REG_DWORD_BIG_ENDIAN,
  REG_LINK,
  REG_MULTI_SZ,
};
/* Macros: */
/** Endpoint address of the Joystick HID reporting IN endpoint. */
#define JOYSTICK_EPADDR_IN (ENDPOINT_DIR_IN | 1)
#define KEYBOARD_EPADDR (ENDPOINT_DIR_IN | 1)

/** Size in bytes of the Keyboard HID reporting IN endpoint. */
#define KEYBOARD_EPSIZE 8
#define REQ_GetOSFeatureDescriptor 0x20

/** Descriptor index for a Microsoft Proprietary Extended Device Compatibility
 * descriptor. */
#define EXTENDED_COMPAT_ID_DESCRIPTOR 0x0004

/** Descriptor index for a Microsoft Proprietary Extended Device Properties OS
 * Feature Descriptor. */
#define EXTENDED_PROPERTIES_DESCRIPTOR 0x0005

/** wchar_t is not available on all platforms */
#if ((ARCH == ARCH_AVR8) || (ARCH == ARCH_XMEGA))
#define WCHAR wchar_t
#else
#define WCHAR uint16_t
#endif

/** Interface & (dummy) Endpoint properties */
#define WCID_IF_NUMBER 0
#define WCID_IN_EPNUM 1
#define WCID_OUT_EPNUM 2
#define WCID_IO_EPSIZE 64

/** Registry values we want to set using the Properties Descriptor */
#define NB_PROPERTIES 2
#define PROPERTY1_NAME L"DeviceInterfaceGUID"
#define PROPERTY1_TYPE REG_SZ
#define PROPERTY1_DATA L"{12341234-1234-1234-1234-123412341234}"
#define PROPERTY2_NAME L"Icons"
#define PROPERTY2_TYPE REG_EXPAND_SZ
#define PROPERTY2_DATA L"%SystemRoot%\\system32\\setupapi.dll,-18"

#define GENERIC_REPORT_SIZE 0
#define GENERIC_EPSIZE 8
#define XBOX_EPSIZE 32

/** Helper Macro for the above. Header size is 14 bytes */
#define PROPERTY_SIZE(idx)                                                     \
  (14 + sizeof(PROPERTY##idx##_NAME) + sizeof(PROPERTY##idx##_DATA))
/* Type Defines: */

typedef struct {
  USB_Descriptor_Header_t Header;
  uint8_t data[15];
} USB_HID_XBOX_Descriptor_HID_t;
#if OUTPUT_TYPE == XINPUT
typedef struct {
  USB_Descriptor_Configuration_Header_t Config;
  USB_Descriptor_Interface_t Interface0;
  USB_HID_XBOX_Descriptor_HID_t XInputUnknown;
  USB_Descriptor_Endpoint_t DataInEndpoint0;
  USB_Descriptor_Endpoint_t DataOutEndpoint0;
} USB_Descriptor_Configuration_t;
#elif OUTPUT_TYPE == KEYBOARD
typedef struct {
  USB_Descriptor_Configuration_Header_t Config;

  // Keyboard HID Interface
  USB_Descriptor_Interface_t HID_Interface;
  USB_HID_Descriptor_HID_t HID_KeyboardHID;
  USB_Descriptor_Endpoint_t HID_ReportINEndpoint;
} USB_Descriptor_Configuration_t;

#endif
/** Enum for the device interface descriptor IDs within the device. Each
 * interface descriptor should have a unique ID index associated with it, which
 * can be used to refer to the interface from other descriptors.
 */
enum InterfaceDescriptors_t {
  INTERFACE_ID_Keyboard = 0, /**< Keyboard interface descriptor ID */
};

/** Enum for the device string descriptor IDs within the device. Each string
 * descriptor should have a unique ID index associated with it, which can be
 * used to refer to the string from other descriptors.
 */
enum StringDescriptors_t {
  STRING_ID_Language =
      0, /**< Supported Languages string descriptor ID (must be zero) */
  STRING_ID_Manufacturer = 1, /**< Manufacturer string ID */
  STRING_ID_Product = 2,      /**< Product string ID */
};
typedef struct {
  USB_Descriptor_Header_t Header;
  WCHAR Signature[7];
  uint8_t VendorCode;
  uint8_t Reserved;
} USB_OSDescriptor_t;

typedef struct {
  uint8_t FirstInterfaceNumber;
  uint8_t Reserved;
  uint8_t CompatibleID[8];
  uint8_t SubCompatibleID[8];
  uint8_t Reserved2[6];
} USB_OSCompatibleSection_t;

typedef struct {
  uint32_t TotalLength;
  uint16_t Version;
  uint16_t Index;
  uint8_t TotalSections;
  uint8_t Reserved[7];
  USB_OSCompatibleSection_t CompatID;
} USB_OSCompatibleIDDescriptor_t;

typedef struct {
  uint16_t Length;
  WCHAR Value[32];
} USB_OSPropertiesName_t;

typedef struct {
  uint32_t Length;
  WCHAR Value[64];
} USB_OSPropertiesData_t;

typedef struct {
  uint32_t Length;
  uint32_t Type;
  USB_OSPropertiesName_t Name;
  USB_OSPropertiesData_t Data;
} USB_OSPropertiesSection_t;

typedef struct {
  uint32_t TotalLength;
  uint16_t Version;
  uint16_t Index;
  uint16_t TotalSections;
} USB_OSPropertiesHeader_t;

typedef struct {
  USB_OSPropertiesHeader_t Header;
  USB_OSPropertiesSection_t Property[NB_PROPERTIES];
} USB_OSPropertiesDescriptor_t;

/* Function Prototypes: */
uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                    const uint16_t wIndex,
                                    const void **const DescriptorAddress)
    ATTR_WARN_UNUSED_RESULT ATTR_NON_NULL_PTR_ARG(3);

uint16_t USB_GetOSFeatureDescriptor(const uint8_t InterfaceNumber,
                                    const uint8_t wIndex,
                                    const uint8_t Recipient,
                                    const void **const DescriptorAddress)
    ATTR_WARN_UNUSED_RESULT ATTR_NON_NULL_PTR_ARG(4);