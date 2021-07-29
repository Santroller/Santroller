#pragma once
#include "datatypes.h"
#include "../descriptors.h"
#include "EmulatedSPI.h"

void setup_response_manager(void);
void process_OUT_report(uint8_t* ReportData, uint8_t ReportSize);