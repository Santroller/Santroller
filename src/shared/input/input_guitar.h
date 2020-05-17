#pragma once
#include "../controller/controller.h"
#include "../Arduino.h"
void guitar_init(void);
void guitar_tick(controller_t* controller);
bool is_guitar(void);
bool is_drum(void);