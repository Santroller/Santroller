#pragma once
#include "../controller/controller.h"
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
void initInputs(void);
void tickInputs(Controller_t* controller);
uint8_t getVelocity(Controller_t* controller, uint8_t offset);
extern uint8_t detectedPin;