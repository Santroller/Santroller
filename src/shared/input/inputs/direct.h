#pragma once
#include "controller/controller.h"
#include "../pins/pins.h"
void initDirectInput(void);
void tickDirectInput(Controller_t *controller);
void findAnalogPin(void);
void findDigitalPin(void);
void stopSearching(void);
extern uint8_t detectedPin;
extern bool foundPin;