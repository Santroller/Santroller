#include "leds/led_mappings.hpp"
#include "profiles/profile.hpp"
#include "utils.h"
#include "emulation/usb/hid_device.h"
#include "managers/inactivity_manager.hpp"

void LedMapping::off()
{
    m_device->forget_written();
    m_device->off();
}

bool LedMapping::s_test_ended = false;

bool LedMapping::take_test_ended()
{
    bool ended = s_test_ended;
    s_test_ended = false;
    return ended;
}

void LedMapping::end_test(bool send_events)
{
    m_test_until = 0;
    if (m_device)
    {
        m_device->off();
    }
    s_test_ended = true;
    m_test_end_unsent = true;
    if (send_events)
    {
        proto_Event event = {which_event : proto_Event_led_tag, event : {led : {m_id, 0, 0}}};
        m_test_end_unsent = !HIDConfigDevice::send_event(event, false);
    }
}

void LedMapping::test(uint32_t duration_ms)
{
    InactivityManager::instance().input_activity(millis());
    if (duration_ms == 0)
    {
        end_test(true);
        return;
    }
    m_test_until = millis() + duration_ms;
    m_test_end_unsent = false;
    if (m_device)
    {
        m_device->test_on();
    }
    proto_Event event = {which_event : proto_Event_led_tag, event : {led : {m_id, UINT16_MAX, UINT16_MAX}}};
    HIDConfigDevice::send_event(event, false);
}

bool LedMapping::tick_test(bool send_events)
{
    // Profiles can be updated without events first (an active one by its console's emulation), so the
    // tool hears the test is over from the next update that sends them, or it shows it running forever
    if (m_test_end_unsent && send_events)
    {
        proto_Event event = {which_event : proto_Event_led_tag, event : {led : {m_id, 0, 0}}};
        m_test_end_unsent = !HIDConfigDevice::send_event(event, false);
    }
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
        end_test(send_events);
        return false;
    }
    // The effect's next frame, unless the host or a test has the LED
    if (m_device && m_device->has_effect() && !overridden())
    {
        m_device->redraw_effect();
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
        if (led && led.get() != this && (led->commanded() || led->is_testing()) && led->m_device->shares_led_with(*m_device))
        {
            return true;
        }
    }
    return false;
}

bool LedMapping::held_by_test() const
{
    if (is_testing())
    {
        return true;
    }
    for (const auto &led : m_profile->leds)
    {
        if (led && led.get() != this && led->is_testing() && led->m_device->shares_led_with(*m_device))
        {
            return true;
        }
    }
    return false;
}
