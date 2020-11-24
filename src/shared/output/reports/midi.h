#pragma once
#include "controller/controller.h"
#include <stdint.h>
void fillMIDIReport(void *ReportData, uint8_t *const ReportSize,
                        Controller_t *controller);