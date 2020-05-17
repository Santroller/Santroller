#pragma once
#include <stdint.h>
#include "controller/controller.h"
void fillXInputReport(void *ReportData, uint16_t *const ReportSize,
                          Controller_t *controller);