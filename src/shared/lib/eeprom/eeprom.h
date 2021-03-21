#pragma once
#include "config/config.h"
#include "config/defaults.h"

extern Configuration_t config;
// extern Configuration_t EEMEM config_pointer;
void loadConfig(void);
void resetConfig(void);
void writeConfigBlock(uint8_t offset, const uint8_t* data, uint16_t len);
void writeConfigByte(uint8_t offset, uint8_t byte);