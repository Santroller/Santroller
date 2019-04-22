#include "Descriptors.h"
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
    
/** Device descriptor structure. This descriptor, located in FLASH memory,
 * describes the overall device characteristics, including the supported USB
 * version, control endpoint size and the number of device configurations. The
 * descriptor is read out by the USB host when the enumeration process begins.
 */

const USB_Descriptor_Device_t PROGMEM DeviceDescriptor = {
    .Header = {.Size = sizeof(USB_Descriptor_Device_t), .Type = DTYPE_Device},

    .USBSpecification = VERSION_BCD(2, 0, 0),
#if OUTPUT_TYPE == XINPUT
    .Class = 0xFF,
    .SubClass = 0xFF,
    .Protocol = 0xFF,
    .Endpoint0Size = 0x40,
#elif OUTPUT_TYPE == KEYBOARD || OUTPUT_TYPE == GAMEPAD
    .Class = USB_CSCP_NoDeviceClass,
    .SubClass = USB_CSCP_NoDeviceSubclass,
    .Protocol = USB_CSCP_NoDeviceProtocol,
    .Endpoint0Size = 0x08,
#endif
    .VendorID = 0x1209,
    .ProductID = 0x2882,
    .ReleaseNumber = 0x3122,

    .ManufacturerStrIndex = 0x01,
    .ProductStrIndex = 0x02,
    .SerialNumStrIndex = 0x03,

    .NumberOfConfigurations = 0x01};

/** This function is called by the library when in device mode, and must be
 * overridden (see library "USB Descriptors" documentation) by the application
 * code so that the address and size of a requested descriptor can be given to
 * the USB library. When the device receives a Get Descriptor request on the
 * control endpoint, this function is called so that the descriptor details can
 * be passed back and the appropriate descriptor sent back to the USB host.
 */
uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                    const uint16_t wIndex,
                                    const void **const DescriptorAddress) {
  const uint8_t DescriptorType = (wValue >> 8);
  const uint8_t DescriptorNumber = (wValue & 0xFF);

  const void *Address = NULL;
  uint16_t Size =
      get_descriptor(DescriptorType, DescriptorNumber, DescriptorAddress);
  if (Size) {
    return Size;
  }

  switch (DescriptorType) {
  case DTYPE_Device:
    Address = &DeviceDescriptor;
    Size = sizeof(DeviceDescriptor);
    break;
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
    }
    break;
  }

  *DescriptorAddress = Address;
  return Size;
}
