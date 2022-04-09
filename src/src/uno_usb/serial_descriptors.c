#include "serial_descriptors.h"
const USB_Descriptor_Device_t PROGMEM DeviceDescriptor =
    {
        .Header = {.Size = sizeof(USB_Descriptor_Device_t), .Type = DTYPE_Device},

        .USBSpecification = VERSION_BCD(1, 1, 0),
        .Class = 0x02,
        .SubClass = 0x00,
        .Protocol = 0x00,

        .Endpoint0Size = FIXED_CONTROL_ENDPOINT_SIZE,

        .VendorID = 0x1209,

        .ProductID = 0x2883,
        .ReleaseNumber = 0x0001,

        .ManufacturerStrIndex = NO_DESCRIPTOR,
        .ProductStrIndex = NO_DESCRIPTOR,
        .SerialNumStrIndex = NO_DESCRIPTOR,

        .NumberOfConfigurations = FIXED_NUM_CONFIGURATIONS};

const USB_Descriptor_Configuration_t PROGMEM ConfigurationDescriptor =
    {
        .Config =
            {
                .Header = {.Size = sizeof(USB_Descriptor_Configuration_Header_t), .Type = DTYPE_Configuration},

                .TotalConfigurationSize = sizeof(USB_Descriptor_Configuration_t),
                .TotalInterfaces = 2,

                .ConfigurationNumber = 1,
                .ConfigurationStrIndex = NO_DESCRIPTOR,

                .ConfigAttributes = 0,

                .MaxPowerConsumption = USB_CONFIG_POWER_MA(100)},

        .CDC_CCI_Interface =
            {
                .Header = {.Size = sizeof(USB_Descriptor_Interface_t), .Type = DTYPE_Interface},

                .InterfaceNumber = 0,
                .AlternateSetting = 0,

                .TotalEndpoints = 1,

                .Class = 0x02,
                .SubClass = 0x02,
                .Protocol = 0x01,

                .InterfaceStrIndex = NO_DESCRIPTOR},

        .CDC_Functional_Header =
            {
                .Header = {.Size = sizeof(USB_CDC_Descriptor_FunctionalHeader_t), .Type = CDC_DTYPE_CSInterface},
                .Subtype = CDC_DSUBTYPE_CSInterface_Header,

                .CDCSpecification = VERSION_BCD(1, 1, 0),
            },

        .CDC_Functional_ACM =
            {
                .Header = {.Size = sizeof(USB_CDC_Descriptor_FunctionalACM_t), .Type = CDC_DTYPE_CSInterface},
                .Subtype = CDC_DSUBTYPE_CSInterface_ACM,

                .Capabilities = 0x06,
            },

        .CDC_Functional_Union =
            {
                .Header = {.Size = sizeof(USB_CDC_Descriptor_FunctionalUnion_t), .Type = CDC_DTYPE_CSInterface},
                .Subtype = CDC_DSUBTYPE_CSInterface_Union,

                .MasterInterfaceNumber = 0,
                .SlaveInterfaceNumber = 1,
            },

        .CDC_NotificationEndpoint =
            {
                .Header = {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},

                .EndpointAddress = CDC_NOTIFICATION,
                .Attributes = (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
                .EndpointSize = ENDPOINT_SIZE,
                .PollingIntervalMS = 0xFF},

        .CDC_DCI_Interface =
            {
                .Header = {.Size = sizeof(USB_Descriptor_Interface_t), .Type = DTYPE_Interface},

                .InterfaceNumber = 1,
                .AlternateSetting = 0,

                .TotalEndpoints = 2,

                .Class = 0x0A,
                .SubClass = 0x00,
                .Protocol = 0x00,

                .InterfaceStrIndex = NO_DESCRIPTOR},

        .CDC_DataOutEndpoint =
            {
                .Header = {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},

                .EndpointAddress = DEVICE_EPADDR_OUT,
                .Attributes = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
                .EndpointSize = SERIAL_ENDPOINT_SIZE,
                .PollingIntervalMS = 0x01},

        .CDC_DataInEndpoint =
            {
                .Header = {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},

                .EndpointAddress = DEVICE_EPADDR_IN,
                .Attributes = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
                .EndpointSize = SERIAL_ENDPOINT_SIZE,
                .PollingIntervalMS = 0x01}};