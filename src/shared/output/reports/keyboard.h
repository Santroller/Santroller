#pragma once
#include "eeprom/eeprom.h"
#include "controller/controller.h"
#include "output/controller_structs.h"
#include "output/descriptors.h"
#include <stdint.h>
#define CHECK_JOY_KEY(joy)                                                     \
  checkJoyKey(keysConfig.joy.neg, keysConfig.joy.pos, controller->joy,       \
              joyThresholdKb, &usedKeys, KeyboardReport)
#define CHECK_TRIGGER_KEY(trigger)                                             \
  checkJoyKey(0, keysConfig.trigger, controller->trigger, triggerThresholdKb, \
              &usedKeys, KeyboardReport)

int joyThresholdKb;
int triggerThresholdKb;
//TODO: Maybe we should overlay this with midi
Keys_t keysConfig;
void checkJoyKey(int neg, int pos, int val, int thresh, uint8_t *used,
                 USB_ID_KeyboardReport_Data_t *KeyboardReport) {
  if (*used < SIMULTANEOUS_KEYS) {
    if (neg && val < -thresh) { KeyboardReport->KeyCode[*used++] = neg; }
    if (pos && val > thresh) { KeyboardReport->KeyCode[*used++] = pos; }
  }
}
void fillKeyboardReport(void *ReportData, uint8_t *const ReportSize,
                        Controller_t *controller) {
  *ReportSize = sizeof(USB_ID_KeyboardReport_Data_t);
  USB_ID_KeyboardReport_Data_t *KeyboardReport =
      (USB_ID_KeyboardReport_Data_t *)ReportData;
  KeyboardReport->rid = REPORT_ID_KBD;
  uint8_t usedKeys = 0;
  uint8_t *keys = (uint8_t *)&keysConfig;
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
void initKeyboard(Configuration_t* config) {
  keysConfig = config->keys;
  joyThresholdKb = config->axis.joyThreshold << 8;
  triggerThresholdKb = config->axis.triggerThreshold << 8;
}