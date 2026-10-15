#include "leds/led_mappings.hpp"
#include "profiles/profile.hpp"
#include "utils.h"
#include "emulation/usb/hid_device.h"
#include "managers/inactivity_manager.hpp"

void LedMapping::off()
{
    m_device->off();
}

void LedMapping::test(uint32_t duration_ms)
{
    InactivityManager::instance().input_activity(millis());
    if (duration_ms == 0)
    {
        m_test_until = 0;
        if (m_device)
        {
            m_device->off();
        }
        proto_Event event = {which_event : proto_Event_led_tag, event : {led : {m_id, 0, 0}}};
        HIDConfigDevice::send_event(event, false);
        return;
    }
    m_test_until = millis() + duration_ms;
    if (m_device)
    {
        m_device->test_on();
    }
    proto_Event event = {which_event : proto_Event_led_tag, event : {led : {m_id, UINT16_MAX, UINT16_MAX}}};
    HIDConfigDevice::send_event(event, false);
}

bool LedMapping::tick_test(bool send_events)
{
    if (m_test_until != 0)
    {
        if (millis() < m_test_until)
        {
            if (m_device)
            {
                m_device->test_on();
            }
            return true;
        }
        else
        {
            m_test_until = 0;
            if (m_device)
            {
                m_device->off();
            }
            if (send_events)
            {
                proto_Event event = {which_event : proto_Event_led_tag, event : {led : {m_id, 0, 0}}};
                HIDConfigDevice::send_event(event, false);
            }
            return false;
        }
    }
    return false;
}

bool LedMapping::is_testing() const
{
    return m_test_until != 0 && millis() < m_test_until;
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
