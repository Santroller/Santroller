#pragma once
#include "../config/eeprom.h"
#include "../controller/controller.h"
#include "bootloader/bootloader.h"
#include "controller_structs.h"
#include <stdbool.h>
extern Controller_t controller;
bool processHIDWriteFeatureReport(uint8_t data_len, uint8_t *data);
void processHIDReadFeatureReport(void);
void writeToUSB(const void *const Buffer, uint16_t Length);