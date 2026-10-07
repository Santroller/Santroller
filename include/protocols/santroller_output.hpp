#pragma once
#include <stdint.h>
#include <string.h>
#include "enums.pb.h"
#include "protocols/stagekit.hpp"

// Santroller output commands (the protocol SDL's Santroller driver speaks): output report
// id 1, then a command byte. Over USB the report id is always the first byte; over BLE
// a host's HOGP stack usually strips it, so the command is first.
#define SANTROLLER_COMMAND_RUMBLE 0x5A     // left, right (stage kit param, command on instruments)
#define SANTROLLER_COMMAND_PLAYER_LED 0x5C // player index, 0 based (0xFF = none)
#define SANTROLLER_COMMAND_RGB_LED 0x5D    // r, g, b

// Capability bits in the Santroller 2 capabilities report (0x10, byte 2) are the
// Capability* values from enums.pb.h. A turntable reports its euphoria LED as the RGB
// indicator LED.

static inline bool santroller_is_command(uint8_t b)
{
    return b == SANTROLLER_COMMAND_RUMBLE || b == SANTROLLER_COMMAND_PLAYER_LED || b == SANTROLLER_COMMAND_RGB_LED;
}

// Host side: tracks what a connected Santroller should be showing and hands out one
// pending command at a time, so callers can send it whenever their transport is free.
struct SantrollerOutputState
{
    bool is_v2 = false;
    uint8_t capabilities = 0;
    SubType subtype = Gamepad;

    void set_rumble(uint8_t left, uint8_t right)
    {
        // on instruments the rumble channel carries stage kit commands instead
        if (subtype_supports_stagekit(subtype))
            return;
        update_rumble(left, right);
    }
    void set_stagekit(uint8_t param, uint8_t command)
    {
        if (subtype_supports_stagekit(subtype) && subtype != DjHeroTurntable)
            update_rumble(param, command);
    }
    void set_euphoria(bool on)
    {
        // a turntable takes equal left/right as the euphoria LED
        if (subtype == DjHeroTurntable)
            update_rumble(on ? 0xFF : 0, on ? 0xFF : 0);
    }
    void set_player(uint8_t player)
    {
        if (player != m_player)
        {
            m_player = player;
            m_player_dirty = true;
        }
    }
    void set_rgb(uint8_t r, uint8_t g, uint8_t b)
    {
        if (r != m_rgb[0] || g != m_rgb[1] || b != m_rgb[2])
        {
            m_rgb[0] = r;
            m_rgb[1] = g;
            m_rgb[2] = b;
            m_rgb_dirty = true;
        }
    }
    // everything should be resent, e.g. after (re)connecting
    void mark_all_dirty()
    {
        m_rumble_dirty = m_player_dirty = m_rgb_dirty = true;
    }

    // Writes the next pending command (report id first) and returns its length, or 0 if
    // nothing is pending. Call commit() with the command byte once it was sent.
    uint8_t peek(uint8_t *buf, uint8_t size) const
    {
        memset(buf, 0, size);
        buf[0] = 0x01;
        if (m_rumble_dirty && supports(rumble_capability()))
        {
            buf[1] = SANTROLLER_COMMAND_RUMBLE;
            buf[2] = m_rumble[0];
            buf[3] = m_rumble[1];
            return 4;
        }
        if (m_player_dirty && supports(CapabilityHasStandardPlayerLeds))
        {
            buf[1] = SANTROLLER_COMMAND_PLAYER_LED;
            buf[2] = (m_player >= 1 && m_player <= 4) ? (uint8_t)(m_player - 1) : 0xFF;
            return 3;
        }
        if (m_rgb_dirty && supports(CapabilityHasRGBIndicatorLed))
        {
            buf[1] = SANTROLLER_COMMAND_RGB_LED;
            memcpy(&buf[2], m_rgb, 3);
            return 5;
        }
        return 0;
    }
    void commit(uint8_t command)
    {
        if (command == SANTROLLER_COMMAND_RUMBLE)
            m_rumble_dirty = false;
        else if (command == SANTROLLER_COMMAND_PLAYER_LED)
            m_player_dirty = false;
        else if (command == SANTROLLER_COMMAND_RGB_LED)
            m_rgb_dirty = false;
    }

    // Santroller 1 has no capabilities report and ignores what it can't do, so send all
    bool supports(uint8_t cap) const { return !is_v2 || (capabilities & cap); }
    bool has_rumble() const { return !subtype_supports_stagekit(subtype) && supports(CapabilityHasRumble); }
    bool has_stagekit() const { return subtype_supports_stagekit(subtype) && subtype != DjHeroTurntable && supports(CapabilityHasInstrumentLeds); }
    bool has_euphoria() const { return subtype == DjHeroTurntable && supports(CapabilityHasRGBIndicatorLed); }

private:
    void update_rumble(uint8_t left, uint8_t right)
    {
        if (left != m_rumble[0] || right != m_rumble[1])
        {
            m_rumble[0] = left;
            m_rumble[1] = right;
            m_rumble_dirty = true;
        }
    }
    uint8_t rumble_capability() const
    {
        if (subtype == DjHeroTurntable)
            return CapabilityHasRGBIndicatorLed;
        return subtype_supports_stagekit(subtype) ? CapabilityHasInstrumentLeds : CapabilityHasRumble;
    }
    uint8_t m_rumble[2] = {};
    uint8_t m_player = 0;
    uint8_t m_rgb[3] = {};
    bool m_rumble_dirty = false;
    bool m_player_dirty = false;
    bool m_rgb_dirty = false;
};
