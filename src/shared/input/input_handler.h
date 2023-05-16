#pragma once
#include <stdbool.h>
#include "../controller/controller.h"
#include "pins/pins.h"
#define CHECK_JOY(joy, neg, pos)                                               \
  if (controller->joy < -joyThreshold) {                           \
    bit_set(controller->buttons, neg);                                          \
  }                                                                            \
  if (controller->joy > joyThreshold) {                            \
    bit_set(controller->buttons, pos);                                          \
  }
void findAnalogPin(void);
void findDigitalPin(void);
void stopSearching(void);
void initInputs(Configuration_t* config);
bool tickInputs(Controller_t* controller);
void setSP(bool sp);
uint8_t getVelocity(Controller_t* controller, uint8_t offset);
extern uint8_t detectedPin;
extern int16_t analogueData[XBOX_AXIS_COUNT];
extern Pin_t pinData[XBOX_BTN_COUNT];