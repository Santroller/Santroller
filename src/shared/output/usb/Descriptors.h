#pragma once

/* Includes: */
#include <LUFA/Drivers/USB/USB.h>

#include <avr/pgmspace.h>

#define HID_EPSIZE 32
#define HID_REPORTSIZE 64
/** Endpoint address of the HID IN endpoint. */
#define HID_EPADDR_IN (ENDPOINT_DIR_IN | 1)
/** Endpoint address of the CDC device-to-host notification IN endpoint. */
#define CDC_NOTIFICATION_EPADDR (ENDPOINT_DIR_IN | 2)

/** Endpoint address of the CDC device-to-host data IN endpoint. */
#define CDC_TX_EPADDR (ENDPOINT_DIR_IN | 3)

/** Endpoint address of the CDC host-to-device data OUT endpoint. */
#define CDC_RX_EPADDR (ENDPOINT_DIR_OUT | 4)
/** Endpoint address of the HID OUT endpoint. */
#define HID_EPADDR_OUT (ENDPOINT_DIR_OUT | 5)

/** Size in bytes of the CDC device-to-host notification IN endpoint. */
#define CDC_NOTIFICATION_EPSIZE 8

/** Size in bytes of the CDC data IN and OUT endpoints. */
		#define CDC_TX_EPSIZE                64
#define CDC_TX_BANK_SIZE 2
		#define CDC_RX_EPSIZE                32
#define CDC_RX_BANK_SIZE 1
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
/** Enum for the device interface descriptor IDs within the device. Each
 * interface descriptor should have a unique ID index associated with it, which
 * can be used to refer to the interface from other descriptors.
 */
enum InterfaceDescriptors_t {
  INTERFACE_ID_HID = 0,     /**< Controller interface descriptor ID */
  INTERFACE_ID_CDC_CCI = 1, /**< CDC CCI interface descriptor ID */
  INTERFACE_ID_CDC_DCI = 2, /**< CDC DCI interface descriptor ID */
};
typedef struct {
  USB_Descriptor_Header_t Header;
  uint8_t reserved[2];
  uint8_t subtype;
  uint8_t reserved2[15];
} USB_HID_XBOX_Descriptor_HID_t;
typedef struct {
  USB_Descriptor_Endpoint_t DataInEndpoint0;
  USB_Descriptor_Endpoint_t DataOutEndpoint0;
} ControllerEndpoints;
typedef struct {
  USB_Descriptor_Configuration_Header_t Config;
  USB_Descriptor_Interface_Association_t CDC_IAD;
  USB_Descriptor_Interface_t CDC_CCI_Interface;
  USB_CDC_Descriptor_FunctionalHeader_t CDC_Functional_Header;
  USB_CDC_Descriptor_FunctionalACM_t CDC_Functional_ACM;
  USB_CDC_Descriptor_FunctionalUnion_t CDC_Functional_Union;
  USB_Descriptor_Endpoint_t CDC_NotificationEndpoint;
  USB_Descriptor_Interface_t CDC_DCI_Interface;
  USB_Descriptor_Endpoint_t CDC_DataOutEndpoint;
  USB_Descriptor_Endpoint_t CDC_DataInEndpoint;
  USB_Descriptor_Interface_t Interface0;
  union {
    struct {
      USB_HID_XBOX_Descriptor_HID_t XInputReserved;
      ControllerEndpoints Endpoints;
    } XInput;
    struct {
      USB_HID_Descriptor_HID_t HIDDescriptor;
      ControllerEndpoints Endpoints;
    } HID;
  } Controller;
} USB_Descriptor_Configuration_t;
extern uint8_t device_type;
extern uint8_t polling_rate;
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