#pragma once
#include <stdint.h>
#include "enums.pb.h"
#include "input.pb.h"

// Guitar fret / strum state is sampled every loop and each change is queued, then
// presented to the outputs one state at a time. A state only advances once a report
// has picked it up and it has been shown for at least the dequeue interval, so a game
// polling at that rate sees every transition, even ones shorter than its frame time.
class InputQueue
{
public:
    static constexpr uint8_t SIZE = 64;
    // Nothing has built a report from the presented state for this long, so nobody is
    // consuming the queue. Collapse it to the latest state instead of replaying a backlog.
    static constexpr uint32_t STALE_US = 100000;

    bool enabled = false;
    uint32_t interval_us = 1000;

    // Which queue bit a mapping's output uses, or -1 if it isn't queued
    static int8_t bit_for_output(const proto_Output &output, SubType subtype);

    void push(uint16_t mask)
    {
        if (mask == m_last_pushed)
        {
            return;
        }
        m_last_pushed = mask;
        if (m_count == SIZE)
        {
            // full: fold this change into the newest entry rather than losing it
            m_ring[(m_head + m_count - 1) % SIZE] = mask;
            return;
        }
        m_ring[(m_head + m_count) % SIZE] = mask;
        m_count++;
    }

    void tick(uint32_t now_us)
    {
        if (!m_count)
        {
            return;
        }
        if (!m_observed)
        {
            if (now_us - m_presented_at > STALE_US)
            {
                present(m_last_pushed, now_us);
                m_count = 0;
            }
            return;
        }
        if (now_us - m_presented_at >= interval_us)
        {
            present(m_ring[m_head], now_us);
            m_head = (m_head + 1) % SIZE;
            m_count--;
        }
    }

    // Called by queued mappings while an output builds a report from them
    bool presented(int8_t bit)
    {
        m_observed = true;
        return m_presented & (1u << bit);
    }

private:
    void present(uint16_t mask, uint32_t now_us)
    {
        m_presented = mask;
        m_presented_at = now_us;
        m_observed = false;
    }

    uint16_t m_ring[SIZE] = {};
    uint8_t m_head = 0;
    uint8_t m_count = 0;
    uint16_t m_last_pushed = 0;
    uint16_t m_presented = 0;
    uint32_t m_presented_at = 0;
    bool m_observed = false;
};
