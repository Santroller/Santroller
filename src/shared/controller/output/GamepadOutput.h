#pragma once

/* Includes: */
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#include "../../../config/config.h"
#include "../lufa/Descriptors.h"
#include "../Controller.h"
#include "Output.h"
#include "Descriptors.h"
#include <LUFA/Drivers/USB/USB.h>
#include <LUFA/Platform/Platform.h>

/** Type define for the gamepad HID report structure, for creating and sending HID reports to the host PC.
 *  This mirrors the layout described to the host in the HID report descriptor, in Descriptors.c.
 */
typedef struct
{
  uint16_t r_x;
  uint16_t r_y;
  uint16_t Button; /**< Bit mask of the currently pressed gamepad buttons */
} USB_GamepadReport_Data_t;
enum InterfaceDescriptors_t
{
  INTERFACE_ID_Gamepad = 0, /**< Gamepad interface descriptor ID */
};
#define JOYSTICK_EPADDR (ENDPOINT_DIR_IN | 1)
#define JOYSTICK_EPSIZE 8
#ifdef __cplusplus
extern "C" {
#endif

extern USB_ClassInfo_HID_Device_t Joystick_HID_Interface;
#ifdef __cplusplus
}
#endif

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

  // Keyboard HID Interface
  USB_Descriptor_Interface_t HID_Interface;
  USB_HID_Descriptor_HID_t HID_GamepadHID;
  USB_Descriptor_Endpoint_t HID_ReportINEndpoint;
} USB_Descriptor_Configuration_t;