#pragma once
#include "../config/eeprom.h"
#include "../controller/controller.h"
#include "bootloader/bootloader.h"
#include "controller_structs.h"
#include <stdbool.h>
extern Controller_t controller;
void processHIDWriteFeatureReport(uint8_t cmd, uint8_t data_len, uint8_t *data);
void processHIDReadFeatureReport(uint8_t cmd);
void writeToUSB(const void *const Buffer, uint16_t Length);
bool handleCommand(uint8_t cmd);