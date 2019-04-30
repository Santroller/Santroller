#pragma once

/* Includes: */
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "../../../config/eeprom.h"
#include "../Controller.h"
#include "../lufa/Descriptors.h"
#include "Descriptors.h"
#include "HidOutput.h"
#include <LUFA/Drivers/USB/USB.h>
#include <LUFA/Platform/Platform.h>

/** Type define for the gamepad HID report structure, for creating and sending
 * HID reports to the host PC. This mirrors the layout described to the host in
 * the HID report descriptor, in Descriptors.c.
 */
typedef struct {
  uint16_t r_x;
  uint16_t r_y;
  uint16_t Button; /**< Bit mask of the currently pressed gamepad buttons */
} USB_GamepadReport_Data_t;

class GamepadOutput : public HIDOutput {
public:
  void update(Controller controller);
  bool hid_create_report(USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo,
                         uint8_t *const ReportID, const uint8_t ReportType,
                         void *ReportData, uint16_t *const ReportSize);
  USB_ClassInfo_HID_Device_t *createHIDInterface();
  const size_t ReportDatatypeSize();
  uint16_t get_hid_descriptor(const uint8_t DescriptorType,
                              const uint8_t DescriptorNumber,
                              const void **const DescriptorAddress,
                              uint8_t *const DescriptorMemorySpace);
};