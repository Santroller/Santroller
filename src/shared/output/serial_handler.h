#pragma once
#include "../controller/controller.h"
#include "bootloader/bootloader.h"
#include "controller_structs.h"
#include <stdbool.h>
extern Controller_t controller;
extern bool isPs3;
void processHIDWriteFeatureReport(uint8_t cmd, uint8_t data_len, const uint8_t *data);
void processHIDWriteFeatureReportControl(uint8_t cmd, uint8_t data_len);
void processHIDReadFeatureReport(uint8_t cmd, uint8_t report, const void* request);
void writeToUSB(const void *const Buffer, uint8_t Length, uint8_t report, const void* request);
bool handleCommand(uint8_t cmd);