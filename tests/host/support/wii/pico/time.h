#pragma once
// The mappings area's fake clock, plus alarms that only fire when a test fires them
#include_next "pico/time.h"
#include <stdbool.h>

typedef int32_t alarm_id_t;
typedef int64_t (*alarm_callback_t)(alarm_id_t id, void *user_data);

#ifdef __cplusplus
extern "C" {
#endif
alarm_id_t add_alarm_in_ms(uint32_t ms, alarm_callback_t callback, void *user_data, bool fire_if_past);
alarm_id_t add_alarm_in_us(uint64_t us, alarm_callback_t callback, void *user_data, bool fire_if_past);
bool cancel_alarm(alarm_id_t alarm_id);
#ifdef __cplusplus
}
#endif

namespace fake_alarm
{
struct Pending
{
    alarm_callback_t callback = nullptr;
    void *user_data = nullptr;
    uint64_t delay_us = 0;
    alarm_id_t id = 0;
};
// The last alarm scheduled and not yet fired or cancelled
extern Pending pending;
// Runs the pending alarm, returns false if there wasn't one
bool fire();
void reset();
} // namespace fake_alarm
