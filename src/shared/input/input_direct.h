#pragma once
#include "../controller/controller.h"
#include "pins/pins.h"
void direct_init(void);
void direct_tick(controller_t *controller);
void find_analog(void);
void find_digital(void);
void stop_searching(void);
extern uint8_t detectedPin;
extern bool pinDetected;