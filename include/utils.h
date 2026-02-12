#pragma once
#include "stdint.h"
#include <pico/time.h>

inline uint32_t millis()
{
    return to_ms_since_boot(get_absolute_time());
}

inline uint32_t micros()
{
    return to_us_since_boot(get_absolute_time());
}
inline float map(float x, float in_min, float in_max, float out_min, float out_max)
{
    float t = (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;

    if (t > out_max)
    {
        return out_max;
    }
    if (t < out_min)
    {
        return out_min;
    }
    return t;
}
inline uint16_t map_16(int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max)
{
    int32_t t = (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    if (t > out_max)
    {
        return out_max;
    }
    if (t < out_min)
    {
        return out_min;
    }
    return t;
}

inline uint8_t revbits(uint8_t b)
{
    b = (b & 0b11110000) >> 4 | (b & 0b00001111) << 4;
    b = (b & 0b11001100) >> 2 | (b & 0b00110011) << 2;
    b = (b & 0b10101010) >> 1 | (b & 0b01010101) << 1;
    return b;
}