#pragma once
#include <stdbool.h>
#include "controller_structs.h"
#include "../config/eeprom.h"
#include "../controller/controller.h"
#include "bootloader/bootloader.h"
extern controller_t controller;
void process_serial(uint8_t data);
void init_serial(void);