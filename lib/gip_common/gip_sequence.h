#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "protocols/xbox_gip.h"

#define GIP_SEQ_MIN 1
#define GIP_SEQ_MAX 255

// Additional GIP message types per MS-GIPUSB Section 3.1.3 Table Messaging-16
#ifndef GIP_CMD_AUDIO_CONTROL
#define GIP_CMD_AUDIO_CONTROL 0x08
#endif

#ifndef GIP_CMD_LARGE_MESSAGE
#define GIP_CMD_LARGE_MESSAGE 0x1F
#endif

#ifndef GIP_CMD_INPUT_OVERFLOW
#define GIP_CMD_INPUT_OVERFLOW 0x26
#endif

#ifndef GIP_CMD_AUDIO_SAMPLES
#define GIP_CMD_AUDIO_SAMPLES 0x60
#endif

#define GIP_VENDOR_POOL_SLOTS 8

typedef struct {
    uint8_t global;     // Global pool: 0x01, 0x02, 0x03, 0x04, 0x05, 0x07, 0x08, 0x0A, 0x1F
    uint8_t security;   // Unique pool: 0x06 (Security Control / Data)
    uint8_t rumble;     // Unique pool: 0x09 (Gamepad Vibration Report)
    uint8_t extended;   // Unique pool: 0x1E (Extended Commands)
    uint8_t input;      // Unique pool: 0x20 (Gamepad Input Report)
    uint8_t overflow;   // Unique pool: 0x26 (Gamepad Overflow Input Report)
    uint8_t audio;      // Unique pool: 0x60 (Audio Render / Capture)
    struct {
        uint8_t cmd;
        uint8_t seq;
    } vendor[GIP_VENDOR_POOL_SLOTS]; // Dynamic pools for other vendor/custom commands
} gip_sequence_pool_t;

/**
 * Checks if a command uses the Global Sequence ID pool
 * per MS-GIPUSB Table Messaging-16.
 */
static inline bool gip_is_global_sequence_pool(uint8_t cmd)
{
    switch (cmd) {
        case GIP_ACK_RESPONSE:       // 0x01: Protocol Control
        case GIP_ANNOUNCE:           // 0x02: Hello Device
        case GIP_KEEPALIVE:          // 0x03: Status Device
        case GIP_DEVICE_DESCRIPTOR:  // 0x04: Metadata Response / Request
        case GIP_SET_STATE:          // 0x05: Set Device State
        case GIP_VIRTUAL_KEYCODE:    // 0x07: Guide Button Status
        case GIP_CMD_AUDIO_CONTROL:  // 0x08: Audio Control
        case GIP_CMD_LED_ON:         // 0x0A: LED Guide Button
        case GIP_CMD_LARGE_MESSAGE:  // 0x1F: Large Message Request / Debug
            return true;
        default:
            return false;
    }
}

/**
 * Advances a rolling 8-bit sequence counter in range [1..255].
 * Sequence 0 is invalid in active GIP communication.
 */
static inline uint8_t gip_sequence_advance(uint8_t *counter)
{
    uint8_t current = *counter;
    if (current == 0) {
        current = 1;
    }
    uint8_t next = current + 1;
    if (next == 0) {
        next = 1;
    }
    *counter = next;
    return current;
}

/**
 * Initialize all sequence ID counters to 1.
 */
static inline void gip_sequence_pool_init(gip_sequence_pool_t *pool)
{
    if (!pool) return;
    pool->global = 1;
    pool->security = 1;
    pool->rumble = 1;
    pool->extended = 1;
    pool->input = 1;
    pool->overflow = 1;
    pool->audio = 1;
    for (size_t i = 0; i < GIP_VENDOR_POOL_SLOTS; i++) {
        pool->vendor[i].cmd = 0;
        pool->vendor[i].seq = 1;
    }
}

/**
 * Allocate and return the next rolling sequence ID for the specified command type,
 * advancing the appropriate pool.
 */
static inline uint8_t gip_sequence_pool_next(gip_sequence_pool_t *pool, uint8_t cmd)
{
    if (!pool) return 1;

    if (gip_is_global_sequence_pool(cmd)) {
        return gip_sequence_advance(&pool->global);
    }

    switch (cmd) {
        case GIP_AUTH:
            return gip_sequence_advance(&pool->security);
        case GIP_CMD_RUMBLE:
            return gip_sequence_advance(&pool->rumble);
        case GIP_FINAL_AUTH:
            return gip_sequence_advance(&pool->extended);
        case GIP_INPUT_REPORT:
            return gip_sequence_advance(&pool->input);
        case GIP_CMD_INPUT_OVERFLOW:
            return gip_sequence_advance(&pool->overflow);
        case GIP_CMD_AUDIO_SAMPLES:
            return gip_sequence_advance(&pool->audio);
        default: {
            for (size_t i = 0; i < GIP_VENDOR_POOL_SLOTS; i++) {
                if (pool->vendor[i].cmd == cmd) {
                    return gip_sequence_advance(&pool->vendor[i].seq);
                }
            }
            for (size_t i = 0; i < GIP_VENDOR_POOL_SLOTS; i++) {
                if (pool->vendor[i].cmd == 0) {
                    pool->vendor[i].cmd = cmd;
                    pool->vendor[i].seq = 1;
                    return gip_sequence_advance(&pool->vendor[i].seq);
                }
            }
            return gip_sequence_advance(&pool->vendor[0].seq);
        }
    }
}

/**
 * Return current sequence ID for the pool without advancing it.
 */
static inline uint8_t gip_sequence_pool_current(const gip_sequence_pool_t *pool, uint8_t cmd)
{
    if (!pool) return 1;

    if (gip_is_global_sequence_pool(cmd)) {
        return pool->global ? pool->global : 1;
    }

    switch (cmd) {
        case GIP_AUTH:
            return pool->security ? pool->security : 1;
        case GIP_CMD_RUMBLE:
            return pool->rumble ? pool->rumble : 1;
        case GIP_FINAL_AUTH:
            return pool->extended ? pool->extended : 1;
        case GIP_INPUT_REPORT:
            return pool->input ? pool->input : 1;
        case GIP_CMD_INPUT_OVERFLOW:
            return pool->overflow ? pool->overflow : 1;
        case GIP_CMD_AUDIO_SAMPLES:
            return pool->audio ? pool->audio : 1;
        default: {
            for (size_t i = 0; i < GIP_VENDOR_POOL_SLOTS; i++) {
                if (pool->vendor[i].cmd == cmd) {
                    return pool->vendor[i].seq ? pool->vendor[i].seq : 1;
                }
            }
            return 1;
        }
    }
}

#ifdef __cplusplus
}
#endif
