#include "emulation/keyboard_mouse.hpp"
#include <string.h>
#include "pico/time.h"

static_assert(sizeof(KeyboardState::last_seen_keys) >= KEYBOARD_REPORT_KEYS, "key history must cover the whole report");
static_assert(ConsumerState::MAX_KEYS == CONSUMER_REPORT_KEYS, "media key state must match the report");

// first of the 8 modifier keys (left ctrl), which are reported as bits instead of keycodes
#define KEY_MODIFIER_FIRST 0xE0

// Mouse mappings are positions (like a stick), so they set a speed rather than a distance.
// Full deflection moves this far per second, independent of how often reports are sent.
#define MOUSE_MAX_SPEED_PX_PER_S 2000
#define MOUSE_MAX_SCROLL_PER_S 20
static const int64_t mouse_speeds[MouseState::AxisCount] = {MOUSE_MAX_SPEED_PX_PER_S, MOUSE_MAX_SPEED_PX_PER_S, MOUSE_MAX_SCROLL_PER_S, MOUSE_MAX_SCROLL_PER_S};
// deflection * microseconds * speed per unit of movement
static const int64_t mouse_unit = 32768LL * 1000000LL;

void KeyboardMouseReports::reset()
{
    memset(&m_last_keyboard, 0, sizeof(m_last_keyboard));
    m_sent_keyboard = false;
    memset(&m_last_consumer, 0, sizeof(m_last_consumer));
    m_last_mouse_buttons = 0;
    memset(m_mouse_acc, 0, sizeof(m_mouse_acc));
    m_last_mouse_us = time_us_32();
}

void KeyboardMouseReports::collect(const std::vector<std::shared_ptr<Profile>> &profiles)
{
    KeyboardReport *report = &m_keyboard;
    memset(report, 0, sizeof(*report));
    memset(&m_consumer, 0, sizeof(m_consumer));
    uint8_t consumer_count = 0;
    m_mouse_buttons = 0;
    int32_t mouse_axes[MouseState::AxisCount] = {0};
    for (const auto &profile : profiles)
    {
        auto &state = profile->keyboard_state;
        // Modifier keys (left / right ctrl, shift, alt, gui) go in the modifier byte, not the key array
        for (uint8_t i = 0; i < 8; i++)
        {
            if (state.is_key_pressed(KEY_MODIFIER_FIRST + i))
            {
                report->modifier |= 1 << i;
                state.clear_key(KEY_MODIFIER_FIRST + i);
            }
        }
        size_t total_pressed = 0;
        for (size_t i = 0; i < 256; i++)
        {
            if (state.is_key_pressed(i))
            {
                total_pressed++;
            }
        }
        if (total_pressed > sizeof(report->keycode))
        {
            memset(report->keycode, 0x01, sizeof(report->keycode));
            memset(state.last_seen_keys, 0, sizeof(state.last_seen_keys));
        }
        else
        {
            // keep keys that were already held in the same slots
            size_t current = 0;
            for (size_t i = 0; i < sizeof(state.last_seen_keys); i++)
            {
                uint8_t key = state.last_seen_keys[i];
                if (key && state.is_key_pressed(key))
                {
                    if (current < sizeof(report->keycode))
                    {
                        report->keycode[current++] = key;
                    }
                    state.clear_key(key);
                }
            }
            for (size_t i = 0; i < 256; i++)
            {
                if (current >= sizeof(report->keycode))
                {
                    break;
                }
                if (state.is_key_pressed(i))
                {
                    report->keycode[current++] = i;
                }
            }
            memcpy(state.last_seen_keys, report->keycode, sizeof(report->keycode));
        }
        // media keys from every profile, ignoring repeats and anything past what the report holds
        for (uint8_t i = 0; i < profile->consumer_state.count; i++)
        {
            uint16_t usage = profile->consumer_state.keys[i];
            bool seen = false;
            for (uint8_t j = 0; j < consumer_count; j++)
            {
                seen |= m_consumer.usage[j] == usage;
            }
            if (!seen && consumer_count < CONSUMER_REPORT_KEYS)
            {
                m_consumer.usage[consumer_count++] = usage;
            }
        }
        m_mouse_buttons |= profile->mouse_state.buttons;
        for (uint8_t i = 0; i < MouseState::AxisCount; i++)
        {
            if (profile->mouse_state.axes[i])
            {
                mouse_axes[i] = profile->mouse_state.axes[i];
            }
        }
    }

    // Build up mouse movement from how long each axis has been deflected
    uint32_t now = time_us_32();
    int64_t elapsed = now - m_last_mouse_us;
    m_last_mouse_us = now;
    for (uint8_t i = 0; i < MouseState::AxisCount; i++)
    {
        m_mouse_acc[i] += mouse_axes[i] * elapsed * mouse_speeds[i];
        // don't let movement pile up while reports can't be sent
        const int64_t limit = INT8_MAX * mouse_unit;
        m_mouse_acc[i] = m_mouse_acc[i] > limit ? limit : m_mouse_acc[i] < -limit ? -limit : m_mouse_acc[i];
    }
}

bool KeyboardMouseReports::keyboard_pending(KeyboardReport &report) const
{
    report = m_keyboard;
    return !m_sent_keyboard || memcmp(&m_last_keyboard, &m_keyboard, sizeof(m_keyboard)) != 0;
}

void KeyboardMouseReports::keyboard_sent(const KeyboardReport &report)
{
    m_last_keyboard = report;
    m_sent_keyboard = true;
}

bool KeyboardMouseReports::consumer_pending(ConsumerReport &report) const
{
    report = m_consumer;
    return memcmp(&m_last_consumer, &m_consumer, sizeof(m_consumer)) != 0;
}

void KeyboardMouseReports::consumer_sent(const ConsumerReport &report)
{
    m_last_consumer = report;
}

bool KeyboardMouseReports::mouse_pending(MouseReport &report) const
{
    report = {};
    report.buttons = m_mouse_buttons;
    int8_t *movement[MouseState::AxisCount] = {&report.x, &report.y, &report.wheel, &report.pan};
    bool moved = false;
    for (uint8_t i = 0; i < MouseState::AxisCount; i++)
    {
        *movement[i] = m_mouse_acc[i] / mouse_unit;
        moved |= *movement[i] != 0;
    }
    return moved || m_mouse_buttons != m_last_mouse_buttons;
}

void KeyboardMouseReports::mouse_sent(const MouseReport &report)
{
    const int8_t movement[MouseState::AxisCount] = {report.x, report.y, report.wheel, report.pan};
    for (uint8_t i = 0; i < MouseState::AxisCount; i++)
    {
        m_mouse_acc[i] -= movement[i] * mouse_unit;
    }
    m_last_mouse_buttons = report.buttons;
}
