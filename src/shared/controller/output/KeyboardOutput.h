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
#include <LUFA/Drivers/USB/USB.h>
#include <LUFA/Platform/Platform.h>
#define SIMULTANEOUS_KEYS 10

extern "C" {
extern uint8_t keys[SIMULTANEOUS_KEYS];
extern uint8_t usedKeys;
extern USB_ClassInfo_HID_Device_t Keyboard_HID_Interface;
bool CALLBACK_HID_Device_CreateHIDReport(
    USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo, uint8_t *const ReportID,
    const uint8_t ReportType, void *ReportData, uint16_t *const ReportSize);
void CALLBACK_HID_Device_ProcessHIDReport(
    USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo, const uint8_t ReportID,
    const uint8_t ReportType, const void *ReportData,
    const uint16_t ReportSize);
}

#define KEYBOARD_EPADDR (ENDPOINT_DIR_IN | 1)

/** Size in bytes of the Keyboard HID reporting IN endpoint. */
#define KEYBOARD_EPSIZE 8
#define GENERIC_REPORT_SIZE 0
#define GENERIC_EPSIZE 8

/** Enum for the device interface descriptor IDs within the device. Each
 * interface descriptor should have a unique ID index associated with it, which
 * can be used to refer to the interface from other descriptors.
 */
enum InterfaceDescriptors_t
{
  INTERFACE_ID_Keyboard = 0, /**< Keyboard interface descriptor ID */
};

typedef struct
{
  USB_Descriptor_Configuration_Header_t Config;

  // Keyboard HID Interface
  USB_Descriptor_Interface_t HID_Interface;
  USB_HID_Descriptor_HID_t HID_KeyboardHID;
  USB_Descriptor_Endpoint_t HID_ReportINEndpoint;
} USB_Descriptor_Configuration_t;