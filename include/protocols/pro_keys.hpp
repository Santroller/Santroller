#pragma once

#include <stdint.h>

constexpr uint8_t PRO_KEYBOARD_DEFAULT_VELOCITY = 64;

inline bool pro_keyboard_key_pressed(uint8_t key1, uint8_t key2, uint8_t key3,
                                    const uint8_t *velocities, int32_t key)
{
    if (key >= 1 && key <= 8)
        return key1 & (1u << (8 - key));
    if (key >= 9 && key <= 16)
        return key2 & (1u << (16 - key));
    if (key >= 17 && key <= 24)
        return key3 & (1u << (24 - key));
    return key == 25 && (velocities[0] & 0x80);
}

inline bool pro_keyboard_any_key_pressed(uint8_t key1, uint8_t key2, uint8_t key3,
                                         const uint8_t *velocities, int32_t key_count)
{
    if (key_count > 25)
        key_count = 25;
    for (int32_t key = 1; key <= key_count; ++key)
    {
        if (pro_keyboard_key_pressed(key1, key2, key3, velocities, key))
            return true;
    }
    return false;
}

inline uint16_t pro_keyboard_key_pressure(uint8_t key1, uint8_t key2, uint8_t key3,
                                          const uint8_t *velocities, int32_t key)
{
    if (!pro_keyboard_key_pressed(key1, key2, key3, velocities, key))
        return 0;

    int32_t velocity_index = 0;
    for (int32_t held_key = 1; held_key <= key; ++held_key)
    {
        if (!pro_keyboard_key_pressed(key1, key2, key3, velocities, held_key))
            continue;
        if (held_key < key)
            ++velocity_index;
    }

    if (velocity_index >= 5)
        return uint16_t(PRO_KEYBOARD_DEFAULT_VELOCITY) << 9;
    return uint16_t(velocities[velocity_index] & 0x7f) << 9;
}

inline uint16_t pro_keyboard_key_range_pressure(uint8_t key1, uint8_t key2, uint8_t key3,
                                                const uint8_t *velocities, int32_t key_count)
{
    if (key_count > 25)
        key_count = 25;
    uint16_t pressure = 0;
    for (int32_t key = 1; key <= key_count; ++key)
    {
        const uint16_t key_pressure = pro_keyboard_key_pressure(key1, key2, key3, velocities, key);
        if (key_pressure > pressure)
            pressure = key_pressure;
    }
    return pressure;
}
