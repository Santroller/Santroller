#pragma once
#include <stdint.h>
#include "controller/controller.h"
void initPS3(void);
void fillPS3Report(void *ReportData, uint8_t *const ReportSize,
                       Controller_t *controller);