#include "Descriptors.h"
#include "wcid.h"
#include <LUFA/Drivers/USB/USB.h>
/** Language descriptor structure. This descriptor, located in FLASH memory, is
 * returned when the host requests the string descriptor with index 0 (the first
 * index). It is actually an array of 16-bit integers, which indicate via the
 * language ID table available at USB.org what languages the device supports for
 * its string descriptors.
 */
const USB_Descriptor_String_t PROGMEM LanguageString =
    USB_STRING_DESCRIPTOR_ARRAY(LANGUAGE_ID_ENG);

/** Manufacturer descriptor string. This is a Unicode string containing the
 * manufacturer's details in human readable form, and is read out upon request
 * by the host when the appropriate string ID is requested, listed in the Device
 *  Descriptor.
 */
const USB_Descriptor_String_t PROGMEM ManufacturerString =
    USB_STRING_DESCRIPTOR(L"sanjay900");

/** Product descriptor string. This is a Unicode string containing the product's
 * details in human readable form, and is read out upon request by the host when
 * the appropriate string ID is requested, listed in the Device Descriptor.
 */
const USB_Descriptor_String_t PROGMEM ProductString =
    USB_STRING_DESCRIPTOR(L"Ardwiino");

const USB_Descriptor_String_t PROGMEM VersionString =
    USB_STRING_DESCRIPTOR(L"1.2");

/* A Microsoft-proprietary extension. String address 0xEE is used by
Windows for "OS Descriptors", which in this case allows us to indicate
that our device has a Compatible ID to provide. */
const USB_OSDescriptor_t PROGMEM OSDescriptorString = {
  Header : {Size : sizeof(USB_OSDescriptor_t), Type : DTYPE_String},
  Signature : {'M', 'S', 'F', 'T', '1', '0', '0'},
  VendorCode : REQ_GetOSFeatureDescriptor,
  Reserved : 0
};

/** This function is called by the library when in device mode, and must be
 * overridden (see library "USB Descriptors" documentation) by the application
 * code so that the address and size of a requested descriptor can be given to
 * the USB library. When the device receives a Get Descriptor request on the
 * control endpoint, this function is called so that the descriptor details can
 * be passed back and the appropriate descriptor sent back to the USB host.
 */
uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                    const uint16_t wIndex,
                                    const void **const DescriptorAddress,
                                    uint8_t *const DescriptorMemorySpace) {
  const uint8_t DescriptorType = (wValue >> 8);
  const uint8_t DescriptorNumber = (wValue & 0xFF);

  uint16_t Size = get_descriptor(DescriptorType, DescriptorNumber,
                                 DescriptorAddress, DescriptorMemorySpace);
  if (Size) {
    return Size;
  }

  const void *Address = NULL;
  switch (DescriptorType) {
  case DTYPE_String:
    switch (DescriptorNumber) {
    case 0x00:
      Address = &LanguageString;
      Size = pgm_read_byte(&LanguageString.Header.Size);
      break;
    case 0x01:
      Address = &ManufacturerString;
      Size = pgm_read_byte(&ManufacturerString.Header.Size);
      break;
    case 0x02:
      Address = &ProductString;
      Size = pgm_read_byte(&ProductString.Header.Size);
      break;
    case 0x03:
      Address = &VersionString;
      Size = pgm_read_byte(&VersionString.Header.Size);
      break;
    case 0xEE:
      Address = &OSDescriptorString;
      Size = pgm_read_byte(&OSDescriptorString.Header.Size);
      break;
    }
    break;
  }

  *DescriptorMemorySpace = MEMSPACE_FLASH;
  *DescriptorAddress = Address;
  return Size;
}
