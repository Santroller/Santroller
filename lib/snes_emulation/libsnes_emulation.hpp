#pragma once
#include <stdint.h>
#include "hardware/pio.h"

// The console counts as present if it latched the pad this recently
#define SNES_LISTEN_TIMEOUT_MS 1000
// How long the controller keeps answering after its last user releases it. Covers a
// config reload tearing down and rebuilding the instance.
#define SNES_RELEASE_GRACE_MS 1000

// Device side of a SNES or NES controller port. The PIO answers the console by itself, the
// CPU only hands it the latest button state.
class SnesEmulation
{
public:
    SnesEmulation(uint8_t clock_pin, uint8_t latch_pin, uint8_t data_pin, bool nes);
    ~SnesEmulation();
    // Start watching latch, without driving the data line until acquired
    void begin();
    void end();
    // Profile instances hold the controller while they exist. Once the last one lets go
    // and nothing re-acquires it within SNES_RELEASE_GRACE_MS, stop answering.
    void acquire();
    void release();
    void tick();
    // Pressed buttons, bit 0 is the first bit the console clocks in
    void set_buttons(uint16_t buttons);
    bool is_communicating() const;
    bool is_nes() const { return m_nes; }

private:
    bool start();
    void stop();
    uint8_t m_clock_pin;
    uint8_t m_latch_pin;
    uint8_t m_data_pin;
    bool m_nes;
    bool m_listening = false;
    PIO m_pio = nullptr;
    uint m_sm = 0;
    uint m_offset = 0;
    uint16_t m_buttons = 0;
    uint8_t m_users = 0;
    bool m_release_pending = false;
    uint32_t m_release_at_ms = 0;
    bool m_seen_latch = false;
    uint32_t m_last_latch_ms = 0;
};
