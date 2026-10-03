#pragma once
#include <stdint.h>
#include "i2c.hpp"

#define XBOX_ONE_AUTH_I2C_ADDRESS 0x20
#define XBOX_ONE_AUTH_MAX_PAYLOAD 1024
#define XBOX_ONE_AUTH_MAX_FRAME (XBOX_ONE_AUTH_MAX_PAYLOAD + 5)
// Largest transfer: [reg, cmd, lenHi, lenLo, payload, crcHi, crcLo] or [reg] + frame
#define XBOX_ONE_AUTH_MAX_TRANSFER (XBOX_ONE_AUTH_MAX_PAYLOAD + 6)

// Driver for the Xbox One auth chip, ported from portal_of_flipper (helpers/pof_security_i2c.c, helpers/gip_auth_chip.c)
// Fully non-blocking: transfers go via DMA, and the protocol advances from tick()
class XboxOneAuth : public I2CDMAInterface
{
public:
    XboxOneAuth(uint8_t block, uint8_t sda, uint8_t scl, uint32_t clock, int8_t reset_pin)
        : interface(block, sda, scl, clock), m_reset_pin(reset_pin) {};
    void begin();
    void end();
    void tick();
    void process_data(uint8_t addr, bool running, bool timeout, bool abort_detected, bool stop_detected);
    inline bool is_ready() { return m_state >= State::Idle; }
    inline bool is_busy() { return m_state > State::Idle; }
    // Queue a GIP auth payload (0x06 data); returns false if busy or it isn't something the chip handles
    bool request(const uint8_t *data, uint16_t len);
    // Valid once has_response() is true, until the next request()
    inline bool has_response() { return m_response_ready; }
    inline const uint8_t *response() { return m_response; }
    inline uint16_t response_length() { return m_assembled; }
    inline void clear_response() { m_response_ready = false; }

private:
    enum class State : uint8_t
    {
        Off,
        Missing,
        ResetLow,
        ResetRelease,
        Probe,
        Startup,
        StartupStatus,
        Idle,
        Send,
        PollStatus,
        ReadFrame,
        ReadNotification,
        SendAck,
    };
    void send(uint8_t command, const uint8_t *payload, uint16_t length);
    void read_register(uint8_t reg, uint8_t *data, uint16_t length);
    void transfer_done(bool ok);
    void process_frame();
    void fail();
    void wait(uint32_t ms);

    I2CMasterInterface interface;
    int8_t m_reset_pin;
    State m_state = State::Off;
    volatile bool m_transfer_pending = false;
    volatile bool m_transfer_done = false;
    volatile bool m_transfer_ok = false;
    bool m_waiting = false;
    uint32_t m_wait_until = 0;
    uint32_t m_wait_start = 0;
    uint32_t m_timeout_ms = 0;
    uint8_t m_attempts = 0;
    uint8_t m_steps = 0;
    uint8_t m_counter = 0;
    uint8_t m_phase = 3;
    uint8_t m_reg = 0;
    uint8_t m_status[4];
    uint16_t m_assembled = 0;
    uint16_t m_expected = 0;
    bool m_response_ready = false;
    uint16_t m_send_length = 0;
    uint16_t m_expected_frame = 0;
    uint8_t m_buffer[XBOX_ONE_AUTH_MAX_TRANSFER];
    uint8_t m_response[XBOX_ONE_AUTH_MAX_PAYLOAD];
    uint16_t m_cmds[XBOX_ONE_AUTH_MAX_TRANSFER];
};
