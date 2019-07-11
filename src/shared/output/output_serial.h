#pragma once
#include "../../shared/output/usb/Descriptors.h"
#include <LUFA/Drivers/USB/USB.h>
#include <LUFA/Drivers/Peripheral/Serial.h>

void serial_configuration_changed(void);
void serial_control_request(void);
void serial_tick(void);
void serial_init(void);
void serial_receive(uint8_t data);