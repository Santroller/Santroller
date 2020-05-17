#include "keyboard.h"
#include "output/descriptors.h"
#include "config/eeprom.h"
int joyThresholdKb;
int triggerThresholdKb;
void checkJoyKey(int neg, int pos, int val, int thresh, uint8_t *used,
                   USB_KeyboardReport_Data_t *KeyboardReport) {
  if (*used < SIMULTANEOUS_KEYS) {
    if (neg && val < -thresh) { KeyboardReport->KeyCode[*used++] = neg; }
    if (pos && val > thresh) { KeyboardReport->KeyCode[*used++] = pos; }
  }
}
void fillKeyboardReport(void *ReportData, uint16_t *const ReportSize,
                            Controller_t *controller) {
  *ReportSize = sizeof(USB_KeyboardReport_Data_t);
  USB_KeyboardReport_Data_t *KeyboardReport =
      (USB_KeyboardReport_Data_t *)ReportData;
  uint8_t usedKeys = 0;
  uint8_t *keys = (uint8_t *)&config.keys;
  for (int i = 0; i <= XBOX_Y && usedKeys < SIMULTANEOUS_KEYS; i++) {
    uint8_t binding = keys[i];
    if (binding && bit_check(controller->buttons, i)) {
      KeyboardReport->KeyCode[usedKeys++] = binding;
    }
  }
  CHECK_JOY_KEY(l_x);
  CHECK_JOY_KEY(l_y);
  CHECK_JOY_KEY(r_x);
  CHECK_JOY_KEY(r_y);
  CHECK_TRIGGER_KEY(lt);
  CHECK_TRIGGER_KEY(rt);
}
void initKeyboard(void) {
  joyThresholdKb = config.axis.joyThreshold << 8;
  triggerThresholdKb = config.axis.triggerThreshold << 8;
}