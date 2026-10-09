#pragma once
// Fake hardware/timer.h and pico/time.h for the config tests: time only moves when a test moves it
#include <stdint.h>

typedef uint64_t absolute_time_t;
typedef int32_t alarm_id_t;

namespace fake_time
{
inline uint64_t now_us = 0;
inline void advance_ms(uint32_t ms) { now_us += uint64_t(ms) * 1000; }
}

struct fake_timer_hw_t
{
    volatile uint32_t timerawl;
};
inline fake_timer_hw_t fake_timer_hw{};
#define timer_hw (&fake_timer_hw)

inline absolute_time_t get_absolute_time() { return fake_time::now_us; }
inline uint32_t to_ms_since_boot(absolute_time_t t) { return uint32_t(t / 1000); }
