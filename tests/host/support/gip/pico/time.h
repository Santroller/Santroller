#pragma once
// Fake pico/time.h for the GIP tests: time only moves when a test (or sleep_ms) moves it
#include <stdint.h>

typedef uint64_t absolute_time_t;

namespace gip_fake_time
{
// Microseconds since boot, set by tests
inline uint64_t now_us = 0;
inline void set_ms(uint32_t ms) { now_us = (uint64_t)ms * 1000; }
inline void advance_ms(uint32_t ms) { now_us += (uint64_t)ms * 1000; }
}

inline absolute_time_t get_absolute_time() { return gip_fake_time::now_us; }
inline uint32_t to_ms_since_boot(absolute_time_t t) { return (uint32_t)(t / 1000); }
inline uint64_t to_us_since_boot(absolute_time_t t) { return t; }
inline uint64_t time_us_64() { return gip_fake_time::now_us; }
inline uint32_t time_us_32() { return (uint32_t)gip_fake_time::now_us; }
inline void sleep_ms(uint32_t ms) { gip_fake_time::advance_ms(ms); }
