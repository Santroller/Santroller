#pragma once
#include "../config/eeprom.h"
#include "../controller/controller.h"
#include <LUFA/Drivers/USB/USB.h>
#include <stdbool.h>
#include "controller_structs.h"


typedef struct {
  void (*control_request)(void);
  void (*create_hid_report)(USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo,
                            uint8_t *const ReportID, const uint8_t ReportType,
                            void *ReportData, uint16_t *const ReportSize);
  uint16_t (*get_descriptor)(const uint8_t DescriptorType,
                             const uint8_t DescriptorNumber,
                             const void **const DescriptorAddress,
                             uint8_t *const DescriptorMemorySpace);
} event_pointers;
event_pointers events;
extern controller_t controller;
extern USB_ClassInfo_HID_Device_t interface;
void output_init(void);
void output_tick(void);
int16_t process_serial(USB_ClassInfo_CDC_Device_t* VirtualSerial_CDC_Interface);
void EVENT_USB_Device_ConfigurationChanged(void);
void EVENT_USB_Device_ControlRequest(void);
void EVENT_USB_Device_StartOfFrame(void);
bool CALLBACK_HID_Device_CreateHIDReport(
    USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo, uint8_t *const ReportID,
    const uint8_t ReportType, void *ReportData, uint16_t *const ReportSize);

void CALLBACK_HID_Device_ProcessHIDReport(
    USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo, const uint8_t ReportID,
    const uint8_t ReportType, const void *ReportData,
    const uint16_t ReportSize);

