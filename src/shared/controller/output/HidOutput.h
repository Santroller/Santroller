
#include <LUFA/Drivers/USB/USB.h>
#include "../../../config/config.h"
#include "Output.h"
#include <avr/wdt.h>
enum InterfaceDescriptors_t
{
  INTERFACE_ID_HID = 0
};
#define HID_EPADDR (ENDPOINT_DIR_IN | 1)
#define HID_EPSIZE 8
#define HID_REPORTSIZE 64
extern const USB_Descriptor_HIDReport_Datatype_t HIDReport_Datatype[];
extern const size_t HIDReport_Datasize;
extern const size_t PrevHIDReport_size;
extern USB_ClassInfo_HID_Device_t HID_Interface;
extern "C" {
bool CALLBACK_HID_Device_CreateHIDReport(
    USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo, uint8_t *const ReportID,
    const uint8_t ReportType, void *ReportData, uint16_t *const ReportSize);

void CALLBACK_HID_Device_ProcessHIDReport(
    USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo, const uint8_t ReportID,
    const uint8_t ReportType, const void *ReportData,
    const uint16_t ReportSize);
}
typedef struct
{
  USB_Descriptor_Configuration_Header_t Config;
  USB_Descriptor_Interface_t HID_Interface;
  USB_HID_Descriptor_HID_t HID_GamepadHID;
  USB_Descriptor_Endpoint_t HID_ReportINEndpoint;
} USB_Descriptor_Configuration_t;