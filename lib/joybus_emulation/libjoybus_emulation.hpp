#pragma once
#include <stdint.h>
#include "hardware/pio.h"
#include "pico/time.h"

// The console counts as present if it sent a command this recently. Consoles probe empty
// ports every frame, so this is a couple of seconds to ride out a game loading.
#define JOYBUS_LISTEN_TIMEOUT_MS 2000
// How long the controller keeps answering after its last user releases it. Covers a
// config reload tearing down and rebuilding the instance, which would otherwise look
// like an unplug to the console.
#define JOYBUS_RELEASE_GRACE_MS 1000

#define GC_REPORT_SIZE 8
#define N64_REPORT_SIZE 4

// Device side of a GameCube or N64 controller port. The PIO marks the end of each command
// and waits for a reply, which the RX interrupt queues as soon as it has seen the whole
// command, so the reply goes out a fixed time after the console's stop bit.
class JoybusEmulation
{
public:
    JoybusEmulation(uint8_t pin, bool n64);
    ~JoybusEmulation();
    // Claim the PIO and start watching the line, without answering until acquired
    bool begin();
    void end();
    // Profile instances hold the controller while they exist. Once the last one lets go
    // and nothing re-acquires it within JOYBUS_RELEASE_GRACE_MS, stop answering.
    void acquire();
    void release();
    void tick();
    // The report sent in reply to a poll (GC_REPORT_SIZE or N64_REPORT_SIZE bytes)
    void set_report(const uint8_t *report);
    bool is_communicating() const;
    bool get_rumble() const { return m_rumble; }
    bool is_n64() const { return m_n64; }

private:
    static void irq_handler();
    void handle_rx();
    void handle_command();
    void reply(const uint32_t *words, uint8_t count);
    uint8_t command_length(uint8_t command) const;

    uint8_t m_pin;
    bool m_n64;
    bool m_started = false;
    PIO m_pio = nullptr;
    uint m_sm = 0;
    uint m_offset = 0;
    int m_dma = -1;

    uint8_t m_users = 0;
    bool m_release_pending = false;
    uint32_t m_release_at_ms = 0;
    volatile bool m_active = false;
    volatile bool m_rumble = false;
    volatile uint32_t m_last_command_ms = 0;
    volatile bool m_seen_command = false;

    uint8_t m_command[35];
    uint8_t m_command_len = 0;
    // Whether a reply was queued for the command being received
    bool m_replied = false;

    // Replies, already in the bit format the PIO program takes
    uint32_t m_probe_words[2];
    uint8_t m_probe_count = 0;
    uint32_t m_origin_words[6];
    uint8_t m_origin_count = 0;
    uint32_t m_report_words[5];
    uint8_t m_report_count = 0;
    // What the DMA is sending, so set_report can't change a reply mid transfer
    uint32_t m_tx_words[6];
};
