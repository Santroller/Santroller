#pragma once

/* Includes: */
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "../../config/eeprom.h"
#include "../Controller.h"
#include "../lufa/Descriptors.h"
#include "../../util.h"
#include "Descriptors.h"
#include "HidOutput.h"
#include <LUFA/Drivers/USB/USB.h>
#include <LUFA/Platform/Platform.h>

/** Type define for the gamepad HID report structure, for creating and sending
 * HID reports to the host PC. This mirrors the layout described to the host in
 * the HID report descriptor, in Descriptors.c.
 */
typedef struct {
  uint8_t reportID;
  uint8_t reserved;
  uint32_t buttons;
  uint8_t lx;
  uint8_t ly;
  uint8_t rx;
  uint8_t ry;
  uint8_t dpad_up_a;
  uint8_t dpad_right_a;
  uint8_t dpad_down_a;
  uint8_t dpad_left_a;
  uint8_t l2_a;
  uint8_t r2_a;
  uint8_t l1_a;
  uint8_t r1_a;
  uint8_t triangle_a;
  uint8_t circle_a;
  uint8_t cross_a;
  uint8_t square_a;
  uint16_t axis_x;
  uint16_t axis_y;
  uint16_t axis_z;
  uint16_t gyroscope;
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