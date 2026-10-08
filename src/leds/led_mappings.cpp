#include "leds/leds.hpp"
#include "instance.hpp"
#include "utils.h"
#include "hardware/pwm.h"
#include <stdio.h>
#include "emulation/usb/hid_device.h"
#include "enums.pb.h"
#include "managers/config_manager.hpp"
#include "usb/auth_broker.h"
#include "devices/bt/bluetooth_status.hpp"
// Host values are 8 bit, LED values span the full 16 bit range
static uint16_t scale8(uint8_t val)
{
    return val * 257;
}

RumbleLedMapping::RumbleLedMapping(std::unique_ptr<LedMappingDevice> device, proto_RumbleLedMapping mapping, std::shared_ptr<Profile> profile, uint32_t id) : LedMapping(std::move(device), profile, id), m_mapping(mapping) {}
StageKitLedMapping::StageKitLedMapping(std::unique_ptr<LedMappingDevice> device, proto_StageKitLedMapping mapping, std::shared_ptr<Profile> profile, uint32_t id) : LedMapping(std::move(device), profile, id), m_mapping(mapping) {}
PlaystationLedMapping::PlaystationLedMapping(std::unique_ptr<LedMappingDevice> device, proto_PlaystationLedMapping mapping, std::shared_ptr<Profile> profile, uint32_t id) : LedMapping(std::move(device), profile, id), m_mapping(mapping) {}
EuphoriaLedMapping::EuphoriaLedMapping(std::unique_ptr<LedMappingDevice> device, proto_EuphoriaLedMapping mapping, std::shared_ptr<Profile> profile, uint32_t id) : LedMapping(std::move(device), profile, id), m_mapping(mapping) {}
PlayerLedMapping::PlayerLedMapping(std::unique_ptr<LedMappingDevice> device, proto_PlayerLedMapping mapping, std::shared_ptr<Profile> profile, uint32_t id) : LedMapping(std::move(device), profile, id), m_mapping(mapping) {}

void RumbleLedMapping::update(bool full_poll, bool send_events)
{
}
void RumbleLedMapping::set_rumble(uint8_t left, uint8_t right)
{
    if (m_mapping.type == RumbleLeft)
    {
        m_device->set_val(scale8(left));
    }
    else if (m_mapping.type == RumbleRight)
    {
        m_device->set_val(scale8(right));
    }
}

void RumbleLedMapping::reload()
{
}
void StageKitLedMapping::update(bool full_poll, bool send_events)
{
}

void StageKitLedMapping::reload()
{
}
void StageKitLedMapping::set_stagekit_led(uint8_t fog, uint8_t strobe, uint8_t blue, uint8_t green, uint8_t yellow, uint8_t red)
{
    switch (m_mapping.type)
    {
    case StageKitFog:
        m_device->set_val(scale8(fog));
        break;
    case StageKitStrobe:
        m_device->set_val(scale8(strobe));
        break;
    default:
    {
        // The stage kit LEDs (0 - 7) this mapping shows, none picked means all of them
        const uint8_t selected = m_mapping.index ? m_mapping.index : 0xFF;
        // Which stage kit LEDs light each colour channel, yellow being red and green
        uint8_t mask_r = 0, mask_g = 0, mask_b = 0;
        switch (m_mapping.type)
        {
        case StageKitBlue:
            mask_b = blue;
            break;
        case StageKitGreen:
            mask_g = green;
            break;
        case StageKitYellow:
            mask_r = mask_g = yellow;
            break;
        case StageKitRed:
            mask_r = red;
            break;
        case StageKitRGBY:
            mask_r = red | yellow;
            mask_g = green | yellow;
            mask_b = blue;
            break;
        default:
            return;
        }
        if (m_mapping.indexMappingMode == StageKitIndexSequential)
        {
            // Each picked stage kit LED drives the next LED of this mapping, in order
            uint8_t led = 0;
            for (int bit = 0; bit < 8 && led < m_device->led_count(); bit++)
            {
                if (!(selected & (1 << bit)))
                {
                    continue;
                }
                m_device->set_val_raw(led++, mask_r & (1 << bit) ? 0xFF : 0, mask_g & (1 << bit) ? 0xFF : 0, mask_b & (1 << bit) ? 0xFF : 0, 255);
            }
        }
        else if (m_mapping.indexMappingMode == StageKitIndexIntensity)
        {
            // Every LED of this mapping is as bright as the share of the picked stage kit LEDs that are on
            const uint8_t total = __builtin_popcount(selected);
            const uint8_t r = __builtin_popcount(mask_r & selected) * 0xFF / total;
            const uint8_t g = __builtin_popcount(mask_g & selected) * 0xFF / total;
            const uint8_t b = __builtin_popcount(mask_b & selected) * 0xFF / total;
            for (int i = 0; i < m_device->led_count(); i++)
            {
                m_device->set_val_raw(i, r, g, b, 255);
            }
        }
        break;
    }
    }
}
void PlaystationLedMapping::update(bool full_poll, bool send_events)
{
}

void PlaystationLedMapping::reload()
{
}
void PlaystationLedMapping::set_lightbar(uint8_t r, uint8_t g, uint8_t b)
{
    for (int i = 0; i < m_device->led_count(); i++)
    {
        m_device->set_val_raw(i, r, g, b, 255);
    }
}
void EuphoriaLedMapping::update(bool full_poll, bool send_events)
{
}

void EuphoriaLedMapping::reload()
{
}
void EuphoriaLedMapping::set_euphoria_led(uint8_t val)
{
    m_device->set_val(scale8(val));
}
void PlayerLedMapping::update(bool full_poll, bool send_events)
{
}
void PlayerLedMapping::set_player_led(uint8_t player)
{
    m_device->set_val(player == m_mapping.playerId ? UINT16_MAX : 0);
}

void PlayerLedMapping::reload()
{
}
void KeyboardLedMapping::set_keyboard_leds(uint8_t leds)
{
    // HID keyboard LED bits: num lock, caps lock, scroll lock
    uint8_t bit = 0;
    switch (m_mapping.type)
    {
    case KeyboardLedNumLock:
        bit = 1 << 0;
        break;
    case KeyboardLedCapsLock:
        bit = 1 << 1;
        break;
    case KeyboardLedScrollLock:
        bit = 1 << 2;
        break;
    }
    m_device->set_val(leds & bit ? UINT16_MAX : 0);
}

void GameFeedbackLedMapping::set_game_feedback(const GameFeedback &feedback)
{
    switch (m_mapping.type)
    {
    case FeedbackStarPowerGauge:
        m_device->set_val(feedback.star_power_active ? 0 : scale8(feedback.star_power_fill));
        break;
    case FeedbackStarPowerActive:
        m_device->set_val(feedback.star_power_active ? scale8(feedback.star_power_fill) : 0);
        break;
    case FeedbackMultiplier:
        m_device->set_val(feedback.multiplier && feedback.multiplier >= m_mapping.value ? UINT16_MAX : 0);
        break;
    case FeedbackSolo:
        m_device->set_val(feedback.solo ? UINT16_MAX : 0);
        break;
    case FeedbackNoteMiss:
        m_device->set_val(feedback.note_miss ? UINT16_MAX : 0);
        break;
    case FeedbackNoteHit:
        m_device->set_val(m_mapping.value < 8 && (feedback.note_hits & (1 << m_mapping.value)) ? UINT16_MAX : 0);
        break;
    }
}

void StatusLedMapping::update(bool full_poll, bool send_events)
{
    auto mode = ConfigManager::instance().get_current_mode();
    bool on = false;
    switch (m_mapping.type)
    {
    case StatusBluetoothConnected:
        on = bluetooth_connected();
        break;
    case StatusAuthenticated:
        on = auth_broker.is_auth_completed(mode);
        break;
    case StatusConsoleMode:
        on = m_mapping.has_mode && mode == m_mapping.mode;
        break;
    }
    m_device->set_val(on ? UINT16_MAX : 0);
}

void InputLedMapping::update(bool full_poll, bool send_events)
{
    uint16_t raw = m_input->tick_analog();
    uint16_t curr = scale(raw);
    if (send_events && ((full_poll || (raw != m_last_val)) && (millis() - m_last_poll) > 10))
    {
        m_last_val = raw;
        proto_Event event = {which_event : proto_Event_led_tag, event : {led : {m_id, raw, curr}}};
        HIDConfigDevice::send_event(event, false);
        m_last_poll = millis();
    }
    if (m_mapping.has_pattern && m_mapping.pattern == PatternHeatmap)
    {
        if (curr)
        {
            if (millis() - m_last_increase > 10)
            {
                m_pos += 2048;
                if (m_pos > UINT16_MAX)
                {
                    m_pos = UINT16_MAX;
                }
            }
            m_last_increase = millis();
        }
        if (!curr && millis() - m_last_decay > 200)
        {
            if (m_pos <= 2048)
            {
                m_pos = 0;
            }
            else
            {
                m_pos -= 1024;
            }
            m_last_decay = millis();
        }
        m_device->set_val(m_pos);
    }
    else
    {
        m_device->set_val(curr);
    }
}
InputLedMapping::InputLedMapping(std::unique_ptr<LedMappingDevice> device, proto_InputLedMapping mapping, std::unique_ptr<Input> input, std::shared_ptr<Profile> profile, uint32_t id) : LedMapping(std::move(device), profile, id), m_input(std::move(input)), m_mapping(mapping)
{
    // Without a range set, the LED follows the whole input range
    if (m_mapping.has_min || m_mapping.has_max)
    {
        m_min = m_mapping.min;
        m_max = m_mapping.max;
    }
}
uint16_t InputLedMapping::scale(uint16_t raw) const
{
    if (m_max == m_min)
    {
        return raw >= m_max ? UINT16_MAX : 0;
    }
    float amount = (float)((int32_t)raw - m_min) / (float)(m_max - m_min);
    if (amount <= 0)
    {
        return 0;
    }
    if (amount >= 1)
    {
        return UINT16_MAX;
    }
    return amount * UINT16_MAX;
}
PatternLedMapping::PatternLedMapping(std::unique_ptr<LedMappingDevice> device, proto_PatternLedMapping mapping, std::shared_ptr<Profile> profile, uint32_t id) : LedMapping(std::move(device), profile, id), m_mapping(mapping), m_speed(mapping.speed ? mapping.speed : 1), m_brightness(mapping.brightness ? mapping.brightness : 1)
{
    m_speed = 21 - m_speed;
}
void PatternLedMapping::update(bool full_poll, bool send_events)
{
    if (millis() < m_next_poll)
    {
        return;
    }
    uint32_t speed = m_speed;
    uint8_t leds = m_device->led_count();
    // If the led doesn't support brightness, then we just stop each channel at a lower brightness
    uint8_t pos_per_chan = m_device->supports_brightness() ? 255 : m_brightness;
    if (m_mapping.pattern == PatternRainbow)
    {
        uint8_t section = pos_per_chan / 3;
        uint8_t section2 = section * 2;
        for (int i = 0; i < leds; i++)
        {
            auto pos = (i * pos_per_chan / leds + m_pos) % pos_per_chan;
            if (pos < section)
            {
                auto g = 0;
                auto r = pos;
                auto b = pos_per_chan - r;
                m_device->set_val_raw(i, r, g, b, m_brightness);
            }
            else if (pos < section2)
            {
                auto g = pos - section;
                auto r = pos_per_chan - g;
                auto b = 0;
                m_device->set_val_raw(i, r, g, b, m_brightness);
            }
            else if (pos)
            {
                auto b = pos - section2;
                auto g = pos_per_chan - b;
                auto r = 0;
                m_device->set_val_raw(i, r, g, b, m_brightness);
            }
        }
        m_pos++;

        if (!m_device->supports_brightness())
        {
            // Speed needs to be scaled in this scenario, as there are less values being looped over
            speed *= 255 / m_brightness;
        }
    }
    else if (m_mapping.pattern == PatternFade)
    {
        m_device->set_val(m_pos);
        if (m_dir)
        {
            m_pos -= 127;
        }
        else
        {
            m_pos += 127;
        }
        if (m_pos >= UINT16_MAX && !m_dir)
        {
            m_dir = true;
            m_pos -= 127;
        }
        if (m_pos == 0 && m_dir)
        {
            m_dir = false;
        }
    }
    m_next_poll = millis() + speed;
}
void StaticLedMapping::update(bool full_poll, bool send_events)
{
    m_device->set_val(UINT16_MAX);
}
