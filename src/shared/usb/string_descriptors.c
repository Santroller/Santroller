#include "string_descriptors.h"
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
const PROGMEM STRING_DESCRIPTOR manufacturerString = USB_DESCRIPTOR_STRING_ARRAY('s', 'a', 'n', 'j', 'a', 'y', '9', '0', '0');
/** Product descriptor string. This is a Unicode string containing the product's
 * details in human readable form, and is read out upon request by the host when
 * the appropriate string ID is requested, listed in the Device Descriptor.
 */
const PROGMEM STRING_DESCRIPTOR productString = USB_DESCRIPTOR_STRING_ARRAY('S', 'a', 'n', 't', 'r', 'o', 'l', 'l', 'e', 'r');

/**
 * Descriptor used by the Xbox 360 to determine if a controller supports authentication
 */
const PROGMEM STRING_DESCRIPTOR xboxString = USB_DESCRIPTOR_STRING_ARRAY(
    'X', 'b', 'o', 'x', ' ', 'S', 'e', 'c', 'u', 'r', 'i', 't', 'y',
    ' ', 'M', 'e', 't', 'h', 'o', 'd', ' ', '3', ',', ' ',
    'V', 'e', 'r', 's', 'i', 'o', 'n', ' ', '1', '.', '0', '0', ',',
    ' ', 0xa9, ' ', '2', '0', '0', '5', ' ',
    'M', 'i', 'c', 'r', 'o', 's', 'o', 'f', 't', ' ',
    'C', 'o', 'r', 'p', 'o', 'r', 'a', 't', 'i', 'o', 'n', '.', ' ',
    'A', 'l', 'l', ' ', 'r', 'i', 'g', 'h', 't', 's', ' ',
    'r', 'e', 's', 'e', 'r', 'v', 'e', 'd', '.');

const PROGMEM STRING_DESCRIPTOR *const descriptorStrings[3] = {
    &languageString, &manufacturerString, &productString};