#pragma once
// Fake pico/time.h for the mapping tests: time only moves when a test moves it
#include <stdint.h>

typedef uint64_t absolute_time_t;

namespace fake_time
{
// Microseconds since boot, set by tests
inline uint64_t now_us = 0;
inline void set_us(uint64_t us) { now_us = us; }
inline void advance_us(uint64_t us) { now_us += us; }
}

inline absolute_time_t get_absolute_time() { return fake_time::now_us; }
inline uint32_t to_ms_since_boot(absolute_time_t t) { return (uint32_t)(t / 1000); }
inline uint64_t to_us_since_boot(absolute_time_t t) { return t; }
inline uint64_t time_us_64() { return fake_time::now_us; }
inline uint32_t time_us_32() { return (uint32_t)fake_time::now_us; }
