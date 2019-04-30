#include "SerialOutput.h"
#include "../../bootloader/Bootloader.h"
void SerialOutput::usb_connect() {}
void SerialOutput::usb_disconnect() {}
/** Standard file stream for the CDC interface when set up, so that the virtual
 * CDC COM port can be used like any regular character stream in the C APIs.
 */
static FILE USBSerialStream;
/** LUFA CDC Class driver interface configuration and state information. This
 * structure is passed to all CDC Class driver functions, so that multiple
 * instances of the same class within a device can be differentiated from one
 * another.
 */
USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface = {
    .Config =
        {
            .ControlInterfaceNumber = INTERFACE_ID_CDC_CCI,
            .DataINEndpoint =
                {
                    .Address = CDC_TX_EPADDR,
                    .Size = CDC_TXRX_EPSIZE,
                    .Banks = 1,
                },
            .DataOUTEndpoint =
                {
                    .Address = CDC_RX_EPADDR,
                    .Size = CDC_TXRX_EPSIZE,
                    .Banks = 1,
                },
            .NotificationEndpoint =
                {
                    .Address = CDC_NOTIFICATION_EPADDR,
                    .Size = CDC_NOTIFICATION_EPSIZE,
                    .Banks = 1,
                },
        },
};

/** Device descriptor structure. This descriptor, located in FLASH memory,
 * describes the overall device characteristics, including the supported USB
 * version, control endpoint size and the number of device configurations. The
 * descriptor is read out by the USB host when the enumeration process begins.
 */

const USB_Descriptor_Device_t PROGMEM DeviceDescriptor = {
    .Header = {.Size = sizeof(USB_Descriptor_Device_t), .Type = DTYPE_Device},

    .USBSpecification = VERSION_BCD(2, 0, 0),
    .Class = CDC_CSCP_CDCClass,
    .SubClass = USB_CSCP_NoDeviceSubclass,
    .Protocol = USB_CSCP_NoDeviceProtocol,
    .Endpoint0Size = 0x08,
    .VendorID = 0x1209,
    .ProductID = 0x2882,
    .ReleaseNumber = 0x3122,

    .ManufacturerStrIndex = 0x01,
    .ProductStrIndex = 0x02,
    .SerialNumStrIndex = 0x03,

    .NumberOfConfigurations = 0x01};

void SerialOutput::init() {
  USB_Init();
  CDC_Device_CreateStream(&VirtualSerial_CDC_Interface, &USBSerialStream);
  sei();
}
void SerialOutput::usb_configuration_changed() {
  CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);
}
void SerialOutput::usb_control_request() {
  CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
}
void SerialOutput::usb_start_of_frame() {}
void SerialOutput::update(Controller controller) {
  wdt_reset();
  char recv = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);
  if (recv == 't') {
      bootloader();
  };
  if (recv == 'f') {
      fputs(STR(F_CPU), &USBSerialStream);
  };
  if (recv == 'c') {
      CDC_Device_SendData(&VirtualSerial_CDC_Interface, &config, sizeof(config_t));
  }
  CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
  USB_USBTask();
}
/** Configuration descriptor structure. This descriptor, located in FLASH
 * memory, describes the usage of the device in one of its supported
 * configurations, including information about any device interfaces and
 * endpoints. The descriptor is read out by the USB host during the enumeration
 * process when selecting a configuration so that the host may correctly
 * communicate with the USB device.
 */
const USB_CDC_Descriptor_Configuration_t PROGMEM ConfigurationDescriptor = {
    .Config = {.Header = {.Size = sizeof(USB_Descriptor_Configuration_Header_t),
                          .Type = DTYPE_Configuration},

               .TotalConfigurationSize =
                   sizeof(USB_CDC_Descriptor_Configuration_t),
               .TotalInterfaces = 2,

               .ConfigurationNumber = 1,
               .ConfigurationStrIndex = NO_DESCRIPTOR,

               .ConfigAttributes =
                   (USB_CONFIG_ATTR_RESERVED | USB_CONFIG_ATTR_SELFPOWERED),

               .MaxPowerConsumption = USB_CONFIG_POWER_MA(100)},

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

    .CDC_NotificationEndpoint =
        {.Header = {.Size = sizeof(USB_Descriptor_Endpoint_t),
                    .Type = DTYPE_Endpoint},

         .EndpointAddress = CDC_NOTIFICATION_EPADDR,
         .Attributes =
             (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
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

    .CDC_DataOutEndpoint = {.Header = {.Size =
                                           sizeof(USB_Descriptor_Endpoint_t),
                                       .Type = DTYPE_Endpoint},

                            .EndpointAddress = CDC_RX_EPADDR,
                            .Attributes =
                                (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC |
                                 ENDPOINT_USAGE_DATA),
                            .EndpointSize = CDC_TXRX_EPSIZE,
                            .PollingIntervalMS = 0x05},

    .CDC_DataInEndpoint = {.Header = {.Size = sizeof(USB_Descriptor_Endpoint_t),
                                      .Type = DTYPE_Endpoint},

                           .EndpointAddress = CDC_TX_EPADDR,
                           .Attributes = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC |
                                          ENDPOINT_USAGE_DATA),
                           .EndpointSize = CDC_TXRX_EPSIZE,
                           .PollingIntervalMS = 0x05}};
uint16_t SerialOutput::get_descriptor(const uint8_t DescriptorType,
                                      const uint8_t DescriptorNumber,
                                      const void **const DescriptorAddress,
                                      uint8_t *const DescriptorMemorySpace) {
  uint16_t Size = NO_DESCRIPTOR;
  const void *Address = NULL;
  switch (DescriptorType) {
  case DTYPE_Device:
    Address = &DeviceDescriptor;
    Size = sizeof(DeviceDescriptor);
    break;
  case DTYPE_Configuration:
    Address = &ConfigurationDescriptor;
    Size = sizeof(ConfigurationDescriptor);
    break;
  }
  *DescriptorAddress = Address;
  *DescriptorMemorySpace = MEMSPACE_FLASH;
  return Size;
}

/** CDC class driver callback function the processing of changes to the virtual
 *  control lines sent from the host..
 *
 *  \param[in] CDCInterfaceInfo  Pointer to the CDC class interface
 * configuration structure being referenced
 */
void EVENT_CDC_Device_ControLineStateChanged(
    USB_ClassInfo_CDC_Device_t *const CDCInterfaceInfo) {
  /* You can get changes to the virtual CDC lines in this callback; a common
   use-case is to use the Data Terminal Ready (DTR) flag to enable and
   disable CDC communications in your application when set to avoid the
   application blocking while waiting for a host to become ready and read
   in the pending data from the USB endpoints.
*/
  bool HostReady = (CDCInterfaceInfo->State.ControlLineStates.HostToDevice &
                    CDC_CONTROL_LINE_OUT_DTR) != 0;

  (void)HostReady;
}