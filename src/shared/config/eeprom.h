#pragma once
#include "config.h"
#include "defaults.h"
#include <avr/eeprom.h>

extern Configuration_t config;
extern Configuration_t EEMEM config_pointer;
void loadConfig(void);