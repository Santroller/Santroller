#include "LUFAConfig.h"
#include <LUFA/LUFA/Drivers/USB/USB.h>
#include <avr/pgmspace.h>
#include "endpoints.h"

typedef struct
{
    USB_Descriptor_Configuration_Header_t Config;
    USB_Descriptor_Interface_t CDC_CCI_Interface;
    USB_CDC_Descriptor_FunctionalHeader_t CDC_Functional_Header;
    USB_CDC_Descriptor_FunctionalACM_t CDC_Functional_ACM;
    USB_CDC_Descriptor_FunctionalUnion_t CDC_Functional_Union;
    USB_Descriptor_Endpoint_t CDC_NotificationEndpoint;
    USB_Descriptor_Interface_t CDC_DCI_Interface;
    USB_Descriptor_Endpoint_t CDC_DataOutEndpoint;
    USB_Descriptor_Endpoint_t CDC_DataInEndpoint;
} USB_Descriptor_Configuration_t;

extern const USB_Descriptor_Device_t DeviceDescriptor;

extern const USB_Descriptor_Configuration_t ConfigurationDescriptor;