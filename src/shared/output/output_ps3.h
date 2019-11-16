#pragma once
#include "../util.h"
#include "output_handler.h"
void ps3_init(event_pointers *events);
#define COPY(first,second) \
    bit_write(bit_check(controller.buttons, XBOX_##first), JoystickReport->buttons, \
              SWITCH_##second);
