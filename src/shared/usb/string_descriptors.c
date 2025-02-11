#include "string_descriptors.h"
#include "config.h"
/** Language descriptor structure. This descriptor, located in FLASH memory, is
 * returned when the host requests the string descriptor with index 0 (the first
 * index). It is actually an array of 16-bit integers, which indicate via the
 * language ID table available at USB.org what languages the device supports for
 * its string descriptors.
 */
const PROGMEM STRING_DESCRIPTOR languageString = USB_DESCRIPTOR_STRING_ARRAY(LANGUAGE_ID_ENG);
/** Manufacturer descriptor string. This is a Unicode string containing the
 * manufacturer's details in human readable form, and is read out upon request
 * by the host when the appropriate string ID is requested, listed in the Device
 *  Descriptor.
 */
#ifdef DEVICE_VENDOR
const PROGMEM STRING_DESCRIPTOR manufacturerString = USB_DESCRIPTOR_STRING_UTF(DEVICE_VENDOR);
#else
const PROGMEM STRING_DESCRIPTOR manufacturerString = USB_DESCRIPTOR_STRING_UTF("sanjay900");
#endif
/** Product descriptor string. This is a Unicode string containing the product's
 * details in human readable form, and is read out upon request by the host when
 * the appropriate string ID is requested, listed in the Device Descriptor.
 */
#ifdef DEVICE_PRODUCT
const PROGMEM STRING_DESCRIPTOR productString = USB_DESCRIPTOR_STRING_UTF(DEVICE_PRODUCT);
#else
const PROGMEM STRING_DESCRIPTOR productString = USB_DESCRIPTOR_STRING_UTF("Santroller");
#endif

/**
 * Descriptor used by the Xbox 360 to determine if a controller supports authentication
 */
const PROGMEM STRING_DESCRIPTOR xboxString = USB_DESCRIPTOR_STRING_UTF("Xbox Security Method 3, Version 1.00, \xa9 2005 Microsoft Corporation. All rights reserved.");

const PROGMEM STRING_DESCRIPTOR *const descriptorStrings[3] = {
    &languageString, &manufacturerString, &productString};