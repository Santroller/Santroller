#pragma once
#include <stdbool.h>
#include "controller_structs.h"
#include "../config/eeprom.h"
#include "../controller/controller.h"
#include "bootloader/bootloader.h"
extern Controller_t controller;
void processHIDWriteFeatureReport(uint8_t report, uint8_t data_len, uint8_t *data);
void processHIDReadFeatureReport(uint8_t report);