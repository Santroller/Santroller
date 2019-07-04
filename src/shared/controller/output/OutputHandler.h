#pragma once
#include "../../config/eeprom.h"
#include "../Controller.h"
#include "Output.h"
#include "XInputOutput.h"
#include "KeyboardOutput.h"
#include "GamepadOutput.h"
#include "../../util.h"
#include "../../io/bootloader/Bootloader.h"
class OutputHandler {
public:
  void process(Controller *controller);
  void init();
};

extern "C" {
void EVENT_USB_Device_Connect(void);
void EVENT_USB_Device_Disconnect(void);
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
}