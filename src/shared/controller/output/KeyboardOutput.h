#pragma once
#include "../../util.h"
#include "HidOutput.h"
#include "Output.h"
#include <LUFA/Drivers/USB/USB.h>
#include <avr/wdt.h>
#include <stdint.h>
#define SIMULTANEOUS_KEYS 6

#define CHECK_JOY(joy)                                                         \
  if (config.keys.joy.neg && controller.joy < -(int)config.threshold_joy) {    \
    keys[usedKeys++] = config.keys.joy.neg;                                    \
  }                                                                            \
  if (config.keys.joy.pos && controller.joy > (int)config.threshold_joy) {     \
    keys[usedKeys++] = config.keys.joy.pos;                                    \
  }

#define CHECK_TRIGGER(trigger)                                                 \
  if (config.keys.trigger &&                                                   \
      controller.trigger > (int)config.threshold_trigger) {                    \
    keys[usedKeys++] = config.keys.trigger;                                    \
  }
extern "C" {
extern uint8_t keys[SIMULTANEOUS_KEYS];
extern uint8_t usedKeys;
}
class KeyboardOutput : public HIDOutput {
public:
  void update(Controller controller);
  bool check_key(const uint8_t key);
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