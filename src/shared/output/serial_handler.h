#pragma once
#include <LUFA/Drivers/USB/USB.h>
#include <stdbool.h>
#include "controller_structs.h"
#include "../config/eeprom.h"
#include "../controller/controller.h"
#include "bootloader/bootloader.h"
extern controller_t controller;
int16_t process_serial(USB_ClassInfo_CDC_Device_t *VirtualSerial_CDC_Interface);