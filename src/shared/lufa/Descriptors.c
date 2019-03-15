/** \file
 *
 *  USB Device Descriptors, for library use when in USB device mode. Descriptors
 * are special computer-readable structures which the host requests upon device
 * enumeration, to determine the device's capabilities and functions.
 */

#include "Descriptors.h"
/** Device descriptor structure. This descriptor, located in FLASH memory,
 * describes the overall device characteristics, including the supported USB
 * version, control endpoint size and the number of device configurations. The
 * descriptor is read out by the USB host when the enumeration process begins.
 */
const USB_Descriptor_Device_t PROGMEM DeviceDescriptor = {
    .Header = {.Size = sizeof(USB_Descriptor_Device_t), .Type = DTYPE_Device},

    .USBSpecification = VERSION_BCD(1, 0, 0),
    #if OUTPUT_TYPE == XINPUT
    .Endpoint0Size = 0x40,
    .Class = 0xFF,
    .SubClass = 0xFF,
    .Protocol = 0xFF,
    #elif OUTPUT_TYPE == KEYBOARD
    .Endpoint0Size = KEYBOARD_EPSIZE,
    .Class = USB_CSCP_NoDeviceClass,
    .SubClass = USB_CSCP_NoDeviceSubclass,
    .Protocol = USB_CSCP_NoDeviceProtocol,
    #endif
    .VendorID = 0x1209,
    .ProductID = 0x2883,
    .ReleaseNumber = VERSION_BCD(0, 0, 1),

    .ManufacturerStrIndex = 0x01,
    .ProductStrIndex = 0x02,
    .SerialNumStrIndex = 0x03,

    .NumberOfConfigurations = 0x01};
const USB_Descriptor_HIDReport_Datatype_t PROGMEM GenericReport[] = {
    HID_DESCRIPTOR_VENDOR(0x00, 0x01, 0x02, 0x03, GENERIC_REPORT_SIZE)};
/** Configuration descriptor structure. This descriptor, located in FLASH
 * memory, describes the usage of the device in one of its supported
 * configurations, including information about any device interfaces and
 * endpoints. The descriptor is read out by the USB host during the enumeration
 * process when selecting a configuration so that the host may correctly
 * communicate with the USB device.
 */
#if OUTPUT_TYPE==XINPUT
const USB_Descriptor_Configuration_t PROGMEM ConfigurationDescriptor = {
    .Config = {.Header = {.Size = sizeof(USB_Descriptor_Configuration_Header_t),
                          .Type = DTYPE_Configuration},

               .TotalConfigurationSize = sizeof(USB_Descriptor_Configuration_t),
               .TotalInterfaces = 1,

               .ConfigurationNumber = 1,
               .ConfigurationStrIndex = NO_DESCRIPTOR,

               .ConfigAttributes = USB_CONFIG_ATTR_REMOTEWAKEUP,

               .MaxPowerConsumption = USB_CONFIG_POWER_MA(500)},

    .Interface0 = {.Header = {.Size = sizeof(USB_Descriptor_Interface_t),
                              .Type = DTYPE_Interface},

                   .InterfaceNumber = 0,
                   .AlternateSetting = 0x00,

                   .TotalEndpoints = 2,

                   .Class = 0xFF,
                   .SubClass = 0x5D,
                   .Protocol = 0x01,

                   .InterfaceStrIndex = NO_DESCRIPTOR},

    .HID0 = {.Header = {.Size = sizeof(USB_HID_XBOX_Descriptor_HID_t),
                        .Type = HID_DTYPE_HID},

             .HIDSpec = VERSION_BCD(1, 0, 0),
             .CountryCode = 0x00,
             .TotalReportDescriptors = 0x25,
             .HIDReportType0 = 0x81,
             .HIDReportLength0 = 20,
             .HIDReportType1 = 0x00,
             .HIDReportLength1 = 0,
             .HIDReportType2 = 0x13,
             .HIDReportLength2 = 0x0801,
             .HIDReportType3 = 0x00,
             .HIDReportLength3 = 0x00},

    .DataInEndpoint0 = {.Header = {.Size = sizeof(USB_Descriptor_Endpoint_t),
                                   .Type = DTYPE_Endpoint},

                        .EndpointAddress = 0x81,
                        .Attributes = EP_TYPE_INTERRUPT,
                        .EndpointSize = XBOX_EPSIZE,
                        .PollingIntervalMS = POLL_RATE},
    .DataOutEndpoint0 = {.Header = {.Size = sizeof(USB_Descriptor_Endpoint_t),
                                    .Type = DTYPE_Endpoint},

                         .EndpointAddress = 0x01,
                         .Attributes = EP_TYPE_INTERRUPT,
                         .EndpointSize = XBOX_EPSIZE,
                         .PollingIntervalMS = 0x08},
};

#elif OUTPUT_TYPE==KEYBOARD
/** HID class report descriptor. This is a special descriptor constructed with
 * values from the USBIF HID class specification to describe the reports and
 * capabilities of the HID device. This descriptor is parsed by the host and its
 * contents used to determine what data (and in what encoding) the device will
 * send, and what it may be sent back from the host. Refer to the HID
 * specification for more details on HID report descriptors.
 */
const USB_Descriptor_HIDReport_Datatype_t PROGMEM KeyboardReport[] = {
    /* Use the HID class driver's standard Keyboard report.
     *   Max simultaneous keys: 6
     */
    HID_DESCRIPTOR_KEYBOARD(6)};

const USB_Descriptor_Configuration_t PROGMEM ConfigurationDescriptor = {
    .Config = {.Header = {.Size = sizeof(USB_Descriptor_Configuration_Header_t),
                          .Type = DTYPE_Configuration},

               .TotalConfigurationSize = sizeof(USB_Descriptor_Configuration_t),
               .TotalInterfaces = 1,

               .ConfigurationNumber = 1,
               .ConfigurationStrIndex = NO_DESCRIPTOR,

               .ConfigAttributes =
                   (USB_CONFIG_ATTR_RESERVED | USB_CONFIG_ATTR_SELFPOWERED),

               .MaxPowerConsumption = USB_CONFIG_POWER_MA(500)},

    .HID_Interface = {.Header = {.Size = sizeof(USB_Descriptor_Interface_t),
                                 .Type = DTYPE_Interface},

                      .InterfaceNumber = INTERFACE_ID_Keyboard,
                      .AlternateSetting = 0x00,

                      .TotalEndpoints = 1,

                      .Class = HID_CSCP_HIDClass,
                      .SubClass = HID_CSCP_BootSubclass,
                      .Protocol = HID_CSCP_KeyboardBootProtocol,

                      .InterfaceStrIndex = NO_DESCRIPTOR},

    .HID_KeyboardHID = {.Header = {.Size = sizeof(USB_HID_Descriptor_HID_t),
                                   .Type = HID_DTYPE_HID},

                        .HIDSpec = VERSION_BCD(1, 1, 1),
                        .CountryCode = 0x00,
                        .TotalReportDescriptors = 1,
                        .HIDReportType = HID_DTYPE_Report,
                        .HIDReportLength = sizeof(KeyboardReport)},

    .HID_ReportINEndpoint = {.Header = {.Size =
                                            sizeof(USB_Descriptor_Endpoint_t),
                                        .Type = DTYPE_Endpoint},

                             .EndpointAddress = KEYBOARD_EPADDR,
                             .Attributes =
                                 (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC |
                                  ENDPOINT_USAGE_DATA),
                             .EndpointSize = KEYBOARD_EPSIZE,
                             .PollingIntervalMS = POLL_RATE},
};
#endif

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
    USB_STRING_DESCRIPTOR(L"1.1");

const USB_OSDescriptor_t PROGMEM OSDescriptorString = {
  Header : {Size : sizeof(USB_OSDescriptor_t), Type : DTYPE_String},
  Signature : L"MSFT100",
  VendorCode : REQ_GetOSFeatureDescriptor
};

const USB_OSCompatibleIDDescriptor_t PROGMEM DevCompatIDs = {
  TotalLength : sizeof(USB_OSCompatibleIDDescriptor_t),
  Version : 0x0100,
  Index : EXTENDED_COMPAT_ID_DESCRIPTOR,
  TotalSections : 1,
  CompatID : {
    FirstInterfaceNumber : WCID_IF_NUMBER,
    Reserved : 0x01,
    CompatibleID : "XUSB10"
  }
};

const USB_OSPropertiesDescriptor_t PROGMEM DevProperties = {
    .Header = {
      TotalLength : sizeof(USB_OSPropertiesHeader_t) + PROPERTY_SIZE(1) +
          PROPERTY_SIZE(2),
      Version : 0x0100,
      Index : EXTENDED_PROPERTIES_DESCRIPTOR,
      TotalSections : NB_PROPERTIES
    },

    .Property = {
        {
          Length : PROPERTY_SIZE(1),
          Type : PROPERTY1_TYPE,
          Name : {Length : sizeof(PROPERTY1_NAME), Value : PROPERTY1_NAME},
          Data : {Length : sizeof(PROPERTY1_DATA), Value : PROPERTY1_DATA}
        },
        {
          Length : PROPERTY_SIZE(2),
          Type : PROPERTY2_TYPE,
          Name : {Length : sizeof(PROPERTY2_NAME), Value : PROPERTY2_NAME},
          Data : {Length : sizeof(PROPERTY2_DATA), Value : PROPERTY2_DATA}
        },
    }};
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
  uint16_t Size = NO_DESCRIPTOR;

  switch (DescriptorType) {
  case DTYPE_Device:
    Address = &DeviceDescriptor;
    Size = sizeof(DeviceDescriptor);
    break;
  case DTYPE_Configuration:
    Address = &ConfigurationDescriptor;
    Size = sizeof(ConfigurationDescriptor);
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
#if OUTPUT_TYPE==XINPUT
    case 0xEE:
      /* A Microsoft-proprietary extension. String address 0xEE is used by
Windows for "OS Descriptors", which in this case allows us to indicate
that our device has a Compatible ID to provide. */
      Address = &OSDescriptorString;
      Size = pgm_read_byte(&OSDescriptorString.Header.Size);
      break;
#endif
    }
    break;
#if OUTPUT_TYPE==KEYBOARD
  case HID_DTYPE_HID:
    Address = &ConfigurationDescriptor.HID_KeyboardHID;
    Size = sizeof(USB_HID_Descriptor_HID_t);
    break;
  case HID_DTYPE_Report:
    Address = &KeyboardReport;
    Size = sizeof(KeyboardReport);
    break;
#endif
  }

  *DescriptorAddress = Address;
  return Size;
}
uint16_t USB_GetOSFeatureDescriptor(const uint8_t InterfaceNumber,
                                    const uint8_t wIndex,
                                    const uint8_t Recipient,
                                    const void **const DescriptorAddress) {
  const void *Address = NULL;
  uint16_t Size = NO_DESCRIPTOR;

  /* Check if an OS Feature Descriptor is being requested */
  switch (wIndex) {
  case EXTENDED_COMPAT_ID_DESCRIPTOR:
    if (Recipient ==
        REQREC_DEVICE) { /* Ignore InterfaceNumber as this is a
                                                                                Device Request */
      Address = &DevCompatIDs;
      Size = DevCompatIDs.TotalLength;
    }
    break;
  case EXTENDED_PROPERTIES_DESCRIPTOR:
    if ((InterfaceNumber == WCID_IF_NUMBER) &&
        ((Recipient == REQREC_INTERFACE) || (Recipient == REQREC_DEVICE))) {
      Address = &DevProperties;
      Size = DevProperties.Header.TotalLength;
    }
    break;
  }

  *DescriptorAddress = Address;
  return Size;
}