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
#include "../../util.h"
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
  uint16_t buttons : 15;
  uint8_t hat;

  // left and right analog sticks, 0x00 left/up, 0x80 middle, 0xff right/down

  uint8_t l_x;
  uint8_t l_y;
  uint8_t r_x;
  uint8_t r_y;

	// Gonna assume these are button analog values for the d-pad.
	// NOTE: NOT EVEN SURE THIS IS RIGHT, OR IN THE CORRECT ORDER
	uint8_t right_axis;
	uint8_t left_axis;
	uint8_t up_axis;
	uint8_t down_axis;

	// button axis, 0x00 = unpressed, 0xff = fully pressed

	uint8_t triangle_axis;
	uint8_t circle_axis;
	uint8_t cross_axis;
	uint8_t square_axis;

	uint8_t l1_axis;
	uint8_t r1_axis;
	uint8_t l2_axis;
	uint8_t r2_axis;
} USB_GamepadReport_Data_t;

class GamepadOutput : public HIDOutput {
public:
  void update(Controller controller);
  bool hid_create_report(USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo,
                         uint8_t *const ReportID, const uint8_t ReportType,
                         void *ReportData, uint16_t *const ReportSize);
  USB_ClassInfo_HID_Device_t *createHIDInterface();
  const size_t ReportDatatypeSize();
  void usb_control_request();
  uint16_t get_hid_descriptor(const uint8_t DescriptorType,
                              const uint8_t DescriptorNumber,
                              const void **const DescriptorAddress,
                              uint8_t *const DescriptorMemorySpace);
};