#pragma once
#include "../config/eeprom.h"
#include "../controller/controller.h"
#include <LUFA/Drivers/USB/USB.h>
#include <stdbool.h>
typedef struct {
  void (*tick)(controller_t);
  void (*configuration_changed)(void);
  void (*control_request)(void);
  void (*start_of_frame)(void);
  void (*process_hid_report)(USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo,
                             const uint8_t ReportID, const uint8_t ReportType,
                             const void *ReportData, const uint16_t ReportSize);
  bool (*create_hid_report)(USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo,
                            uint8_t *const ReportID, const uint8_t ReportType,
                            void *ReportData, uint16_t *const ReportSize);
  uint16_t (*get_descriptor)(const uint8_t DescriptorType,
                            const uint8_t DescriptorNumber,
                            const void **const DescriptorAddress,
                            uint8_t *const DescriptorMemorySpace);
} event_pointers;
event_pointers events;
void output_init(void);
void output_tick(controller_t controller);
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