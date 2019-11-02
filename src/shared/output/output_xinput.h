#pragma once
#include "../controller/controller.h"
#include "../output_handler.h"
#include <LUFA/Drivers/USB/USB.h>
#include <stdint.h>
#include "usb/Descriptors.h"
/* Enums */
#define XBOX_EPSIZE 32

void xinput_init(event_pointers *);
void xinput_tick(controller_t);