#pragma once
#include <stdint.h>
#include "hardware/pio.h"

// Host side of the joybus line, reading an N64 or GameCube controller.
// Transfers run on DMA, so tick() never blocks waiting on the controller.

// GameCube poll response, byte 0
#define GC_MASK_A (0x1)
#define GC_MASK_B (0x1 << 1)
#define GC_MASK_X (0x1 << 2)
#define GC_MASK_Y (0x1 << 3)
#define GC_MASK_START (0x1 << 4)
// GameCube poll response, byte 1
#define GC_MASK_DPAD_LEFT (0x1)
#define GC_MASK_DPAD_RIGHT (0x1 << 1)
#define GC_MASK_DPAD_DOWN (0x1 << 2)
#define GC_MASK_DPAD_UP (0x1 << 3)
#define GC_MASK_Z (0x1 << 4)
#define GC_MASK_R (0x1 << 5)
#define GC_MASK_L (0x1 << 6)

// N64 poll response, byte 0
#define N64_MASK_A (0x1 << 7)
#define N64_MASK_B (0x1 << 6)
#define N64_MASK_Z (0x1 << 5)
#define N64_MASK_START (0x1 << 4)
#define N64_MASK_DPAD_UP (0x1 << 3)
#define N64_MASK_DPAD_DOWN (0x1 << 2)
#define N64_MASK_DPAD_LEFT (0x1 << 1)
#define N64_MASK_DPAD_RIGHT (0x1)
// N64 poll response, byte 1
#define N64_MASK_RESET (0x1 << 7)
#define N64_MASK_L (0x1 << 5)
#define N64_MASK_R (0x1 << 4)
#define N64_MASK_C_UP (0x1 << 3)
#define N64_MASK_C_DOWN (0x1 << 2)
#define N64_MASK_C_LEFT (0x1 << 1)
#define N64_MASK_C_RIGHT (0x1)

enum class JoybusControllerKind : uint8_t
{
    None,
    N64,
    GameCube,
};

class JoybusController
{
public:
    explicit JoybusController(uint8_t pin);
    ~JoybusController();
    bool begin();
    void end();
    void tick();
    JoybusControllerKind kind() const { return m_kind; }
    // The last poll response: 8 bytes for a GameCube controller, 4 for an N64 controller
    const uint8_t *state() const { return m_state; }
    void set_rumble(bool rumble);

private:
    enum class Step : uint8_t
    {
        Probe,
        GcOrigin,
        GcPoll,
        N64PakInit,
        N64Poll,
        N64Rumble,
    };
    void restart_sm();
    void start_transfer(const uint8_t *request, uint8_t request_len, uint8_t response_len);
    void finish_transfer(bool ok);
    void disconnected();

    uint8_t m_pin;
    bool m_started = false;
    PIO m_pio = nullptr;
    uint m_sm = 0;
    uint m_offset = 0;
    int m_dma_tx = -1;
    int m_dma_rx = -1;

    JoybusControllerKind m_kind = JoybusControllerKind::None;
    Step m_step = Step::Probe;
    bool m_busy = false;
    uint32_t m_transfer_start_us = 0;
    uint32_t m_transfer_timeout_us = 0;
    uint8_t m_response_len = 0;
    uint8_t m_failures = 0;
    bool m_rumble = false;
    bool m_rumble_sent = false;

    uint32_t m_tx[36];
    uint32_t m_rx[11];
    uint8_t m_state[8] = {};
};
