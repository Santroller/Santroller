#pragma once
#include "controller/controller.h"
#include <stdint.h>
#include <LUFA/Common/Common.h>

void fillMouseReport(void *ReportData, uint16_t *const ReportSize,
                     Controller_t *controller);