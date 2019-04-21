#pragma once
/* Includes: */
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <string.h>

#include "../../../config/config.h"
extern "C" {
#include "../lufa/Descriptors.h"
}
#include "Output.h"
#include "stdint.h"

#define XBOX_DPAD_UP 0
#define XBOX_DPAD_DOWN 1
#define XBOX_DPAD_LEFT 2
#define XBOX_DPAD_RIGHT 3
#define XBOX_START 4
#define XBOX_BACK 5
#define XBOX_LEFT_STICK 6
#define XBOX_RIGHT_STICK 7

// digital_buttons_2
#define XBOX_LB 0
#define XBOX_RB 1
#define XBOX_HOME 2
#define XBOX_A 4
#define XBOX_B 5
#define XBOX_X 6
#define XBOX_Y 7