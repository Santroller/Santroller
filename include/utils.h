#pragma once
#include "stdint.h"
#include <pico/time.h>

uint32_t millis() {
    return to_ms_since_boot(get_absolute_time());
}

uint32_t micros() {
    return to_us_since_boot(get_absolute_time());
}