#pragma once

/* Includes: */
#include <LUFA/Drivers/USB/USB.h>

#include <avr/pgmspace.h>

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
extern uint16_t get_descriptor(const uint8_t DescriptorType,
                               const uint8_t DescriptorNumber,
                               const void **const DescriptorAddress,
                               uint8_t *const DescriptorMemorySpace);
uint16_t USB_GetOSFeatureDescriptor(const uint8_t InterfaceNumber,
                                    const uint8_t wIndex,
                                    const uint8_t Recipient,
                                    const void **const DescriptorAddress)
    ATTR_WARN_UNUSED_RESULT ATTR_NON_NULL_PTR_ARG(4);

/* Function Prototypes: */
uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                    const uint16_t wIndex,
                                    const void **const DescriptorAddress,
                                    uint8_t *const DescriptorMemorySpace)
    ATTR_WARN_UNUSED_RESULT ATTR_NON_NULL_PTR_ARG(3);