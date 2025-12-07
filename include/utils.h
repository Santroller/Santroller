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

inline uint8_t revbits(uint8_t b)
{
    b = (b & 0b11110000) >> 4 | (b & 0b00001111) << 4;
    b = (b & 0b11001100) >> 2 | (b & 0b00110011) << 2;
    b = (b & 0b10101010) >> 1 | (b & 0b01010101) << 1;
    return b;
}