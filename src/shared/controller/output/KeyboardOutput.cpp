#include "KeyboardOutput.h"
#if OUTPUT_TYPE == KEYBOARD
#define CHECK_KEY(key)                                                         \
  if (bit_check(controller.buttons, key)) {                                    \
    keys[usedKeys++] = KEY_##key_FRET;                                              \
  }
#define CHECK_KEY2(key, condition)                                             \
  if (bit_check(controller.buttons, key) || condition) {                       \
    keys[usedKeys++] = KEY_##key_FRET;                                              \
  }
void KeyboardOutput::usb_connect() {}
void KeyboardOutput::usb_disconnect() {}

uint8_t keys[SIMULTANEOUS_KEYS];
uint8_t usedKeys = 0;

void KeyboardOutput::init() {
  wdt_enable(WDTO_2S);
  USB_Init();
  sei();
}
void KeyboardOutput::update(Controller controller) {
  USB_USBTask();
  wdt_reset();
  usedKeys = 0;
  if (controller.r_x < -8000) {
    keys[usedKeys++] = KEY_WHAMMY;
  }
  CHECK_KEY(GREEN);
  CHECK_KEY(RED);
  CHECK_KEY(YELLOW);
  CHECK_KEY(BLUE);
  CHECK_KEY(ORANGE);
  CHECK_KEY(UP);
  CHECK_KEY(DOWN);
  CHECK_KEY(LEFT);
  CHECK_KEY(RIGHT);
  CHECK_KEY(START);
  CHECK_KEY2(SELECT, controller.r_y == 32767);
  HID_Device_USBTask(&Keyboard_HID_Interface);
}

void KeyboardOutput::usb_configuration_changed() {
  bool ConfigSuccess = true;
  ConfigSuccess &= HID_Device_ConfigureEndpoints(&Keyboard_HID_Interface);
  USB_Device_EnableSOFEvents();
}

void KeyboardOutput::usb_control_request() {
  HID_Device_ProcessControlRequest(&Keyboard_HID_Interface);
}

void KeyboardOutput::usb_start_of_frame() {
  HID_Device_MillisecondElapsed(&Keyboard_HID_Interface);
}

KeyboardOutput::KeyboardOutput() {}

bool CALLBACK_HID_Device_CreateHIDReport(
    USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo, uint8_t *const ReportID,
    const uint8_t ReportType, void *ReportData, uint16_t *const ReportSize) {
  USB_KeyboardReport_Data_t *KeyboardReport =
      (USB_KeyboardReport_Data_t *)ReportData;
  for (int i = 0; i < usedKeys; i++) {
    KeyboardReport->KeyCode[i] = keys[i];
  }
  *ReportSize = sizeof(USB_KeyboardReport_Data_t);
  return false;
}

void CALLBACK_HID_Device_ProcessHIDReport(
    USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo, const uint8_t ReportID,
    const uint8_t ReportType, const void *ReportData,
    const uint16_t ReportSize) {}
#endif