#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "descriptors.h"
#include "defines.h"

extern ConsoleType_t consoleType;
extern DeviceType_t deviceType;
extern bool guitar;
extern bool drum;

void init(void);
uint8_t tick(uint8_t* data);
void packetReceived(uint8_t* data, uint8_t len);