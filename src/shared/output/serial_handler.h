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
bool getData(uint8_t report);

typedef struct {
    uint8_t ps3id[8];
    uint32_t cpu_freq;
    uint8_t signature[10];
    uint8_t board[10];
    uint8_t detectedPin;
    Configuration_t conf;
} data_t;