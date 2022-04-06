#pragma once
#include "config/config.h"
#include "config/defaults.h"

// extern Configuration_t EEMEM config_pointer;
void loadConfig(Configuration_t* config);
void resetConfig(void);
void writeConfigBlock(uint16_t offset, const uint8_t *data, uint16_t len);
void writeConfigByte(uint16_t offset, uint8_t byte);
void readConfigBlock(uint16_t offset, uint8_t *data, uint16_t len);
extern bool isRF;
extern uint8_t inputType;
extern uint8_t deviceType;
extern uint8_t fullDeviceType;
extern bool typeIsGuitar;
extern bool typeIsDrum;
extern Led_t leds[XBOX_BTN_COUNT + XBOX_AXIS_COUNT];
extern uint8_t drumVelocity[8];