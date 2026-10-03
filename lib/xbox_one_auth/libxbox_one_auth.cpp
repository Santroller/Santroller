#include "libxbox_one_auth.hpp"
#include <string.h>
#include <stdio.h>
#include "hardware/gpio.h"
#include "utils.h"

#define REG_DATA 0x80
#define REG_STATUS 0x82
#define REG_NOTIFICATION 0x90
#define FRAME_TIMEOUT_MS 250
#define POLL_INTERVAL_MS 2
#define MAX_STEPS 16

static uint16_t chip_crc(const uint8_t *data, uint16_t length)
{
    uint16_t crc = 0;
    for (uint16_t i = 0; i < length; i++)
    {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++)
        {
            crc = (crc >> 1) ^ ((crc & 1) ? 0x8408 : 0);
        }
    }
    return crc;
}

void XboxOneAuth::begin()
{
    m_counter = 0;
    m_phase = 3;
    m_response_ready = false;
    m_transfer_pending = false;
    m_transfer_done = false;
    interface.dmaInit(XBOX_ONE_AUTH_I2C_ADDRESS, this);
    m_attempts = 0;
    if (m_reset_pin >= 0)
    {
        // Emulate open drain: drive low, release to the pull-up for high
        gpio_init(m_reset_pin);
        gpio_pull_up(m_reset_pin);
        gpio_put(m_reset_pin, 0);
        gpio_set_dir(m_reset_pin, GPIO_OUT);
        m_state = State::ResetLow;
        wait(2);
    }
    else
    {
        m_state = State::Probe;
    }
}

void XboxOneAuth::end()
{
    m_state = State::Off;
    interface.dmaDeinit(XBOX_ONE_AUTH_I2C_ADDRESS);
    if (m_reset_pin >= 0)
    {
        gpio_deinit(m_reset_pin);
    }
}

// Called from the I2C IRQ, so only record the result; tick() acts on it
void XboxOneAuth::process_data(uint8_t addr, bool running, bool timeout, bool abort_detected, bool stop_detected)
{
    m_transfer_ok = stop_detected && !timeout && !abort_detected;
    m_transfer_done = true;
}

void XboxOneAuth::wait(uint32_t ms)
{
    m_waiting = true;
    m_wait_until = millis() + ms;
}

void XboxOneAuth::send(uint8_t command, const uint8_t *payload, uint16_t length)
{
    // [reg, cmd, lenHi, lenLo, payload..., crcHi, crcLo]; payload may already be in place
    m_buffer[0] = REG_DATA;
    m_buffer[1] = command;
    m_buffer[2] = length >> 8;
    m_buffer[3] = length & 0xff;
    if (length && payload != m_buffer + 4)
        memcpy(m_buffer + 4, payload, length);
    uint16_t crc = chip_crc(m_buffer + 1, length + 3);
    m_buffer[4 + length] = crc >> 8;
    m_buffer[5 + length] = crc & 0xff;
    m_transfer_pending = true;
    m_transfer_done = false;
    interface.dmaWriteRead(XBOX_ONE_AUTH_I2C_ADDRESS, m_buffer, length + 6, nullptr, 0, m_cmds);
}

void XboxOneAuth::read_register(uint8_t reg, uint8_t *data, uint16_t length)
{
    m_reg = reg;
    m_transfer_pending = true;
    m_transfer_done = false;
    interface.dmaWriteRead(XBOX_ONE_AUTH_I2C_ADDRESS, &m_reg, 1, data, length, m_cmds);
}

bool XboxOneAuth::request(const uint8_t *data, uint16_t len)
{
    if (m_state != State::Idle || len < 6 || data[0] != 0)
        return false;
    uint16_t send_length = 6;
    if (data[1] == 0x41)
    {
        send_length += ((uint16_t)data[4] << 8) | data[5];
        if (send_length > len)
            return false;
    }
    else if (data[1] != 0x42 && data[1] != 0x05)
    {
        return false;
    }
    if (send_length > XBOX_ONE_AUTH_MAX_PAYLOAD)
        return false;
    memcpy(m_buffer + 4, data, send_length);
    m_send_length = send_length;
    m_response_ready = false;
    m_state = State::Send;
    return true;
}

void XboxOneAuth::fail()
{
    printf("xone auth chip failed a=%u e=%u\r\n", m_assembled, m_expected);
    m_waiting = false;
    m_state = State::Idle;
}

void XboxOneAuth::process_frame()
{
    uint8_t frame_command = m_buffer[0];
    uint16_t payload_length = ((uint16_t)m_buffer[1] << 8) | m_buffer[2];
    m_phase = (frame_command >> 2) & 3;
    m_timeout_ms = FRAME_TIMEOUT_MS;

    if (frame_command & 0x80)
    {
        m_state = State::ReadNotification;
        read_register(REG_NOTIFICATION, m_status, sizeof(m_status));
        return;
    }
    if (!payload_length)
        return fail();
    uint8_t marker = m_buffer[3] & 7;
    if (!m_assembled)
    {
        if (payload_length > 60 && marker == 1)
        {
            m_expected = (((uint16_t)m_buffer[7] << 8) | m_buffer[8]) + 6;
            if (m_expected > XBOX_ONE_AUTH_MAX_PAYLOAD || payload_length > m_expected)
                return fail();
            memcpy(m_response, m_buffer + 3, payload_length);
            m_response[0] = 0;
            m_assembled = payload_length;
        }
        else if (payload_length <= XBOX_ONE_AUTH_MAX_PAYLOAD && marker != 7)
        {
            memcpy(m_response, m_buffer + 3, payload_length);
            m_assembled = m_expected = payload_length;
        }
        else
        {
            return fail();
        }
    }
    else if ((marker == 2 || marker == 4) && m_assembled + payload_length - 1 <= m_expected)
    {
        memcpy(m_response + m_assembled, m_buffer + 4, payload_length - 1);
        m_assembled += payload_length - 1;
    }
    else
    {
        return fail();
    }
    m_state = State::SendAck;
    send(0x80 | m_phase, nullptr, 0);
}

void XboxOneAuth::transfer_done(bool ok)
{
    switch (m_state)
    {
    case State::Probe:
        if (ok)
        {
            m_state = State::Startup;
            send(0xC0, nullptr, 0);
        }
        else if (++m_attempts < 10)
        {
            wait(20);
        }
        else
        {
            m_state = State::Missing;
            printf("xone auth chip missing\r\n");
        }
        return;
    case State::Startup:
        if (!ok)
        {
            m_state = State::Missing;
            printf("xone auth chip missing\r\n");
            return;
        }
        m_state = State::StartupStatus;
        wait(10);
        return;
    case State::StartupStatus:
        m_state = State::Idle;
        printf("xone auth chip ready\r\n");
        return;
    default:
        break;
    }
    if (!ok)
        return fail();
    switch (m_state)
    {
    case State::Send:
        m_assembled = 0;
        m_expected = 0;
        m_steps = 0;
        m_timeout_ms = FRAME_TIMEOUT_MS;
        m_wait_start = millis();
        m_state = State::PollStatus;
        read_register(REG_STATUS, m_status, sizeof(m_status));
        return;
    case State::PollStatus:
    {
        if (!(m_status[0] & 0x40))
        {
            if (millis() - m_wait_start >= m_timeout_ms)
                return fail();
            wait(POLL_INTERVAL_MS);
            return;
        }
        uint16_t length = ((uint16_t)m_status[2] << 8) | m_status[3];
        if (length < 5 || length > XBOX_ONE_AUTH_MAX_FRAME)
            return fail();
        m_expected_frame = length;
        m_state = State::ReadFrame;
        read_register(REG_DATA, m_buffer, length);
        return;
    }
    case State::ReadFrame:
    {
        uint16_t length = m_expected_frame;
        uint16_t payload_length = ((uint16_t)m_buffer[1] << 8) | m_buffer[2];
        if (payload_length + 5 != length)
            return fail();
        if (chip_crc(m_buffer, length - 2) != (((uint16_t)m_buffer[length - 2] << 8) | m_buffer[length - 1]))
            return fail();
        if (++m_steps > MAX_STEPS)
            return fail();
        process_frame();
        return;
    }
    case State::ReadNotification:
    {
        uint32_t indicated = ((uint32_t)m_status[0] << 24) | ((uint32_t)m_status[1] << 16) | ((uint32_t)m_status[2] << 8) | m_status[3];
        if (indicated > 3000)
            return fail();
        m_timeout_ms = indicated + FRAME_TIMEOUT_MS;
        m_wait_start = millis();
        m_state = State::PollStatus;
        wait(POLL_INTERVAL_MS);
        return;
    }
    case State::SendAck:
        if (m_assembled == m_expected)
        {
            m_response_ready = true;
            m_state = State::Idle;
            return;
        }
        m_timeout_ms = FRAME_TIMEOUT_MS;
        m_wait_start = millis();
        m_state = State::PollStatus;
        read_register(REG_STATUS, m_status, sizeof(m_status));
        return;
    default:
        return;
    }
}

void XboxOneAuth::tick()
{
    interface.tick();
    if (m_transfer_pending)
    {
        if (!m_transfer_done)
            return;
        m_transfer_pending = false;
        transfer_done(m_transfer_ok);
        return;
    }
    if (m_waiting)
    {
        if ((int32_t)(millis() - m_wait_until) < 0)
            return;
        m_waiting = false;
    }
    switch (m_state)
    {
    case State::ResetLow:
        gpio_set_dir(m_reset_pin, GPIO_IN);
        m_state = State::ResetRelease;
        wait(20);
        break;
    case State::ResetRelease:
        m_state = State::Probe;
        break;
    case State::Probe:
        // A status read doubles as the readiness probe (the DMA path can't do zero length transfers)
        read_register(REG_STATUS, m_status, sizeof(m_status));
        break;
    case State::StartupStatus:
        read_register(REG_STATUS, m_status, sizeof(m_status));
        break;
    case State::Send:
    {
        uint8_t command = (m_counter & 0x0c) | (m_phase & 3);
        m_counter = (m_counter + 4) & 0x0c;
        send(command, m_buffer + 4, m_send_length);
        break;
    }
    case State::PollStatus:
        read_register(REG_STATUS, m_status, sizeof(m_status));
        break;
    default:
        break;
    }
}
