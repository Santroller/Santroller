/**
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/timer.h"

#include "quadrature_encoder.pio.h"
#include "quadrature_encoder.h"

//
// ---- quadrature encoder interface example
//
// the PIO program reads phase A/B of a quadrature encoder and increments or
// decrements an internal counter to keep the current absolute step count
// updated. At any point, the main code can query the current count by using
// the quadrature_encoder_*_count functions. The counter is kept in a full
// 32 bit register that just wraps around. Two's complement arithmetic means
// that it can be interpreted as a 32-bit signed or unsigned value, and it will
// work anyway.
//
// As an example, a two wheel robot being controlled at 100Hz, can use two
// state machines to read the two encoders and in the main control loop it can
// simply ask for the current encoder counts to get the absolute step count. It
// can also subtract the values from the last sample to check how many steps
// each wheel as done since the last sample period.
//
// One advantage of this approach is that it requires zero CPU time to keep the
// encoder count updated and because of that it supports very high step rates.
//

QuadratureEncoder::QuadratureEncoder(uint8_t pin, uint8_t divisor) : delta(0), position(0), m_pin(pin), m_divisor(divisor > 0 ? divisor : 1), m_sub_count(0)
{
}

void QuadratureEncoder::begin()
{
    if (m_initialized)
        return;
    if (pio_claim_free_sm_and_add_program_for_gpio_range(&quadrature_encoder_program, &pio, &sm, &m_offset, m_pin, 2, true))
    {
        m_initialized = true;
        quadrature_encoder_program_init(pio, sm, m_pin, 0);
        old_value = quadrature_encoder_get_count(pio, sm);
        delta = 0;
        position = 0;
        m_sub_count = 0;
    }
}
    
void QuadratureEncoder::end()
{
    if (!m_initialized)
        return;
    m_initialized = false;
    quadrature_encoder_program_end(pio, sm);
    pio_remove_program_and_unclaim_sm(&quadrature_encoder_program, pio, sm, m_offset);
}
void QuadratureEncoder::tick()
{
    if (!m_initialized)
        return;
    int new_value = quadrature_encoder_get_count(pio, sm);
    int raw_delta = new_value - old_value;
    old_value = new_value;

    if (m_divisor <= 1)
    {
        delta = raw_delta;
        position += raw_delta;
    }
    else
    {
        m_sub_count += raw_delta;
        int step = m_sub_count / m_divisor;
        m_sub_count %= m_divisor;
        delta = step;
        position += step;
    }
}