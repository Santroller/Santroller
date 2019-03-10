#pragma once
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "../../config/config.h"
#include "../Controller.h"
#include "../lufa/Descriptors.h"
#include "Descriptors.h"
#include <LUFA/Drivers/USB/USB.h>
#include <LUFA/Platform/Platform.h>

#ifdef __cplusplus
extern "C" {
#endif

extern USB_ClassInfo_HID_Device_t Keyboard_HID_Interface;
#ifdef __cplusplus
}
#endif