#include "output_keyboard.h"
#include "../util.h"
#include "usb/Descriptors.h"
#include "output_hid.h"

static uint8_t prev_keyboard_report[sizeof(USB_KeyboardReport_Data_t)];
const USB_Descriptor_HIDReport_Datatype_t PROGMEM
    keyboard_report_descriptor[] = {HID_DESCRIPTOR_KEYBOARD(SIMULTANEOUS_KEYS)};

bool keyboard_create_report(USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo,
                            uint8_t *const ReportID, const uint8_t ReportType,
                            void *ReportData, uint16_t *const ReportSize) {
  USB_KeyboardReport_Data_t *KeyboardReport =
      (USB_KeyboardReport_Data_t *)ReportData;
  uint8_t usedKeys = 0;
  uint8_t *keys = (uint8_t *)&config.keys;
  for (int i = 0; i <= XBOX_Y && usedKeys < SIMULTANEOUS_KEYS; i++) {
    uint8_t binding = keys[i];
    if (binding && bit_check(last_controller.buttons, i)) {
      KeyboardReport->KeyCode[usedKeys++] = binding;
    }
  }
  CHECK_JOY_KEY(l_x);
  CHECK_JOY_KEY(l_y);
  CHECK_JOY_KEY(r_x);
  CHECK_JOY_KEY(r_y);
  CHECK_TRIGGER_KEY(lt);
  CHECK_TRIGGER_KEY(rt);
  *ReportSize = sizeof(USB_KeyboardReport_Data_t);
  return false;
}
void keyboard_init(event_pointers *events, USB_ClassInfo_HID_Device_t *hid_device) {
  events->create_hid_report = keyboard_create_report;
  hid_report_address = keyboard_report_descriptor;
  hid_report_size = sizeof(keyboard_report_descriptor);
  hid_device->Config.PrevReportINBuffer = &prev_keyboard_report;
  hid_device->Config.PrevReportINBufferSize = sizeof(prev_keyboard_report);
}