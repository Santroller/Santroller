#pragma once
#include "stdint.h"
#include <pico/time.h>

inline uint32_t millis() {
    return to_ms_since_boot(get_absolute_time());
}

inline uint32_t micros() {
    return to_us_since_boot(get_absolute_time());
}
inline float map(float x, float in_min, float in_max, float out_min, float out_max) {

  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;

}