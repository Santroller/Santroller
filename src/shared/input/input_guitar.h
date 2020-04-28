#pragma once
#include "../controller/controller.h"
void guitar_init(void);
void guitar_tick(controller_t* controller);
bool is_not_guitar(void);
bool is_drum(void);