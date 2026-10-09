#pragma once
// Fake utils.h: only the fake clock
#include "pico/time.h"
inline uint32_t millis() { return to_ms_since_boot(get_absolute_time()); }
