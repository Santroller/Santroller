#pragma once
#include <stdint.h>
#include "hardware/pio.h"

// Fat (original) and Slim 360 RF modules use the same module-clocked 2 wire bus,
// but the slim module needs a different init command, a hold time after each
// falling edge, and an extra trailing bit on sync.
class Xbox360Rf
{
public:
    Xbox360Rf(uint8_t data_pin, uint8_t clock_pin, bool slim);
    bool begin();
    void end();
    void send_init();
    void send_sync();
    // Feed queued commands to the state machine, call regularly from the main loop
    void tick();

private:
    struct Command
    {
        uint16_t bits;
        uint8_t count;
    };
    void queue(uint16_t bits, uint8_t count);
    bool idle();
    void reset_sm();

    static constexpr uint8_t QUEUE_SIZE = 4;
    // gap between commands, as used by both reference implementations
    static constexpr uint32_t COMMAND_GAP_MS = 50;
    // nothing clocking the bus (module missing / unpowered), give up on the command
    static constexpr uint32_t COMMAND_TIMEOUT_MS = 500;

    uint8_t m_data_pin;
    uint8_t m_clock_pin;
    bool m_slim;
    PIO m_pio = nullptr;
    uint m_sm = 0;
    uint m_offset = 0;
    Command m_queue[QUEUE_SIZE];
    uint8_t m_head = 0;
    uint8_t m_count = 0;
    bool m_sending = false;
    uint32_t m_sent_at = 0;
    uint32_t m_next_at = 0;
};
