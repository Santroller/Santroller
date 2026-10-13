#include "leds/led_mappings.hpp"
#include "profiles/profile.hpp"
#include "utils.h"

void LedMapping::off()
{
    m_device->off();
}

void LedMapping::begin_feedback()
{
    m_device->start_tracking();
}

void LedMapping::end_feedback()
{
    uint32_t written = m_device->written();
    if (written == m_last_feedback)
    {
        return;
    }
    // The first feedback only counts if it lights something, as the host hasn't asked for
    // anything yet when everything starts off
    if (m_last_feedback != LedMappingDevice::NOTHING_WRITTEN || m_device->lit())
    {
        m_commanded_at = millis();
        m_was_commanded = true;
    }
    m_last_feedback = written;
}

bool LedMapping::commanded() const
{
    return m_was_commanded && (millis() - m_commanded_at) < COMMAND_HOLD_MS;
}

bool LedMapping::overridden() const
{
    for (const auto &led : m_profile->leds)
    {
        if (led && led.get() != this && led->commanded() && led->m_device->shares_led_with(*m_device))
        {
            return true;
        }
    }
    return false;
}
