#pragma once
#include <stdint.h>
#include "controller/controller.h"
void fillXInputReport(void *ReportData, uint8_t *const ReportSize,
                          Controller_t *controller);