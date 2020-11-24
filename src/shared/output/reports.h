#pragma once
#include "config/defines.h"
#include "config/eeprom.h"
#include "controller/controller.h"
#include "controller_structs.h"
#include "stdint.h"

extern void (*fillReport)(void *ReportData, uint8_t *const ReportSize,
                      Controller_t* controller);
void initReports(void);