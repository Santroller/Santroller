#pragma once
#include "HidOutput.h"
#include "Output.h"
#include <LUFA/Drivers/USB/USB.h>
#include <avr/wdt.h>
#include <stdint.h>
#define SIMULTANEOUS_KEYS 6

#define CHECK_KEY(key)                                                         \
  if (bit_check(controller.buttons, key)) {                                    \
    keys[usedKeys++] = KEY_##key;                                              \
  }
#define CHECK_FRET(key)                                                        \
  if (bit_check(controller.buttons, key)) {                                    \
    keys[usedKeys++] = KEY_##key##_FRET;                                       \
  }
#define CHECK_KEY2(key, condition)                                             \
  if (bit_check(controller.buttons, key) || condition) {                       \
    keys[usedKeys++] = KEY_##key;                                              \
  }
extern "C" {
extern uint8_t keys[SIMULTANEOUS_KEYS];
extern uint8_t usedKeys;
}
class KeyboardOutput : public HIDOutput {
public:
  void update(Controller controller);
  bool hid_create_report(USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo,
                         uint8_t *const ReportID, const uint8_t ReportType,
                         void *ReportData, uint16_t *const ReportSize);
};