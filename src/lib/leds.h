#pragma once
#include "pin.h"
#include "led_colours.h"
#define NUM_LEDS XBOX_BTN_COUNT + XBOX_AXIS_COUNT
void tickLEDs(Input_t** pins);