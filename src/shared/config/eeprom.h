#pragma once
#include "config.h"
#include "defaults.h"
#include <avr/eeprom.h>

extern config_t config;
extern config_t EEMEM config_pointer;
void load_config(void);
void write_config(void);