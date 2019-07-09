#include "Descriptors.h"
#include "../output_handler.h"
#include "wcid.h"
#include <LUFA/Drivers/USB/USB.h>

const void *hid_report_address;
uint16_t hid_report_size;
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

USB_Descriptor_Configuration_t ConfigurationDescriptor = {
  Config : {
    Header : {
      Size : sizeof(USB_Descriptor_Configuration_Header_t),
      Type : DTYPE_Configuration
    },

    TotalConfigurationSize : sizeof(USB_Descriptor_Configuration_t),
    TotalInterfaces : 3,

    ConfigurationNumber : 1,
    ConfigurationStrIndex : NO_DESCRIPTOR,

    ConfigAttributes : USB_CONFIG_ATTR_RESERVED,

    MaxPowerConsumption : USB_CONFIG_POWER_MA(500)
  },

  HID_Interface : {
    Header :
        {Size : sizeof(USB_Descriptor_Interface_t), Type : DTYPE_Interface},

    InterfaceNumber : INTERFACE_ID_HID,
    AlternateSetting : 0x00,

    TotalEndpoints : 2,

    Class : HID_CSCP_HIDClass,
    SubClass : HID_CSCP_NonBootSubclass,
    Protocol : HID_CSCP_NonBootProtocol,

    InterfaceStrIndex : NO_DESCRIPTOR
  },

  XInputUnknown : {
    Header : {Size : sizeof(USB_HID_XBOX_Descriptor_HID_t), Type : 0x21},
    {0x10, 0x01},
    0,
    {0x25, 0x81, 0x14, 0x03, 0x03, 0x03, 0x04, 0x13, 0x02, 0x08, 0x03, 0x03}
  },
  HID_GamepadHID : {
    Header : {Size : sizeof(USB_HID_Descriptor_HID_t), Type : HID_DTYPE_HID},

    HIDSpec : VERSION_BCD(1, 1, 1),
    CountryCode : 0x00,
    TotalReportDescriptors : 2,
    HIDReportType : HID_DTYPE_Report,
    HIDReportLength : 0
  },

  HID_ReportINEndpoint : {
    Header : {Size : sizeof(USB_Descriptor_Endpoint_t), Type : DTYPE_Endpoint},

    EndpointAddress : HID_EPADDR_IN,
    Attributes :
        (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
    EndpointSize : HID_EPSIZE,
    PollingIntervalMS : POLL_RATE
  },

  HID_ReportOUTEndpoint : {
    Header : {Size : sizeof(USB_Descriptor_Endpoint_t), Type : DTYPE_Endpoint},

    EndpointAddress : HID_EPADDR_OUT,
    Attributes :
        (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
    EndpointSize : HID_EPSIZE,
    PollingIntervalMS : POLL_RATE
  },
  .CDC_IAD = {.Header = {.Size = sizeof(USB_Descriptor_Interface_Association_t),
                         .Type = DTYPE_InterfaceAssociation},

              .FirstInterfaceIndex = INTERFACE_ID_CDC_CCI,
              .TotalInterfaces = 2,

              .Class = CDC_CSCP_CDCClass,
              .SubClass = CDC_CSCP_ACMSubclass,
              .Protocol = CDC_CSCP_ATCommandProtocol,

              .IADStrIndex = NO_DESCRIPTOR},

  .CDC_CCI_Interface = {.Header = {.Size = sizeof(USB_Descriptor_Interface_t),
                                   .Type = DTYPE_Interface},

                        .InterfaceNumber = INTERFACE_ID_CDC_CCI,
                        .AlternateSetting = 0,

                        .TotalEndpoints = 1,

                        .Class = CDC_CSCP_CDCClass,
                        .SubClass = CDC_CSCP_ACMSubclass,
                        .Protocol = CDC_CSCP_ATCommandProtocol,

                        .InterfaceStrIndex = NO_DESCRIPTOR},

  .CDC_Functional_Header =
      {
          .Header = {.Size = sizeof(USB_CDC_Descriptor_FunctionalHeader_t),
                     .Type = CDC_DTYPE_CSInterface},
          .Subtype = CDC_DSUBTYPE_CSInterface_Header,

          .CDCSpecification = VERSION_BCD(1, 1, 0),
      },

  .CDC_Functional_ACM =
      {
          .Header = {.Size = sizeof(USB_CDC_Descriptor_FunctionalACM_t),
                     .Type = CDC_DTYPE_CSInterface},
          .Subtype = CDC_DSUBTYPE_CSInterface_ACM,

          .Capabilities = 0x06,
      },

  .CDC_Functional_Union =
      {
          .Header = {.Size = sizeof(USB_CDC_Descriptor_FunctionalUnion_t),
                     .Type = CDC_DTYPE_CSInterface},
          .Subtype = CDC_DSUBTYPE_CSInterface_Union,

          .MasterInterfaceNumber = INTERFACE_ID_CDC_CCI,
          .SlaveInterfaceNumber = INTERFACE_ID_CDC_DCI,
      },

  .CDC_NotificationEndpoint = {.Header = {.Size =
                                              sizeof(USB_Descriptor_Endpoint_t),
                                          .Type = DTYPE_Endpoint},

                               .EndpointAddress = CDC_NOTIFICATION_EPADDR,
                               .Attributes =
                                   (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC |
                                    ENDPOINT_USAGE_DATA),
                               .EndpointSize = CDC_NOTIFICATION_EPSIZE,
                               .PollingIntervalMS = 0xFF},

  .CDC_DCI_Interface = {.Header = {.Size = sizeof(USB_Descriptor_Interface_t),
                                   .Type = DTYPE_Interface},

                        .InterfaceNumber = INTERFACE_ID_CDC_DCI,
                        .AlternateSetting = 0,

                        .TotalEndpoints = 2,

                        .Class = CDC_CSCP_CDCDataClass,
                        .SubClass = CDC_CSCP_NoDataSubclass,
                        .Protocol = CDC_CSCP_NoDataProtocol,

                        .InterfaceStrIndex = NO_DESCRIPTOR},

  .CDC_DataOutEndpoint = {.Header = {.Size = sizeof(USB_Descriptor_Endpoint_t),
                                     .Type = DTYPE_Endpoint},

                          .EndpointAddress = CDC_RX_EPADDR,
                          .Attributes = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC |
                                         ENDPOINT_USAGE_DATA),
                          .EndpointSize = CDC_TXRX_EPSIZE,
                          .PollingIntervalMS = 0x05},

  .CDC_DataInEndpoint = {.Header = {.Size = sizeof(USB_Descriptor_Endpoint_t),
                                    .Type = DTYPE_Endpoint},

                         .EndpointAddress = CDC_TX_EPADDR,
                         .Attributes = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC |
                                        ENDPOINT_USAGE_DATA),
                         .EndpointSize = CDC_TXRX_EPSIZE,
                         .PollingIntervalMS = 0x05},
};
USB_Descriptor_Device_t DeviceDescriptor = {
  Header : {Size : sizeof(USB_Descriptor_Device_t), Type : DTYPE_Device},

  USBSpecification : VERSION_BCD(2, 0, 0),
  Class : USB_CSCP_NoDeviceClass,
  SubClass : USB_CSCP_NoDeviceSubclass,
  Protocol : USB_CSCP_NoDeviceProtocol,
  Endpoint0Size : HID_EPSIZE,
  VendorID : 0x1209,
  ProductID : 0x2882,
  ReleaseNumber : 0x3122,

  ManufacturerStrIndex : 0x01,
  ProductStrIndex : 0x02,
  SerialNumStrIndex : 0x03,

  NumberOfConfigurations : 0x01
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

  uint16_t Size = NO_DESCRIPTOR;

  const void *Address = NULL;
  uint8_t MemorySpace = MEMSPACE_FLASH;
  switch (DescriptorType) {
  case DTYPE_Device:
    MemorySpace = MEMSPACE_RAM;
    Address = &DeviceDescriptor;
    Size = sizeof(DeviceDescriptor);
    break;
  case DTYPE_Configuration:
    MemorySpace = MEMSPACE_RAM;
    Address = &ConfigurationDescriptor;
    Size = sizeof(ConfigurationDescriptor);
    break;
  case HID_DTYPE_Report:
    if (hid_report_address) {
      Address = hid_report_address;
      Size = hid_report_size;
    }
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
    case 0xEE:
      Address = &OSDescriptorString;
      Size = pgm_read_byte(&OSDescriptorString.Header.Size);
      break;
    }
    break;
  }

  *DescriptorMemorySpace = MemorySpace;
  *DescriptorAddress = Address;
  return Size;
}
