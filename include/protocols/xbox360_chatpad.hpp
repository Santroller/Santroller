#pragma once
// The Xbox 360 chatpad's reports, as a keyboard's HID keycodes. Scancodes are from xboxdrv's chatpad.cpp, and the
// green / orange layers from a US chatpad's keycaps
#include <stdint.h>
#include <string.h>
#include "class/hid/hid.h"

// The chatpad's own scancodes, which aren't HID ones
static const uint8_t chatpad_keycodes[][2] = {
    {0x17, HID_KEY_1}, {0x16, HID_KEY_2}, {0x15, HID_KEY_3}, {0x14, HID_KEY_4}, {0x13, HID_KEY_5},
    {0x12, HID_KEY_6}, {0x11, HID_KEY_7}, {0x67, HID_KEY_8}, {0x66, HID_KEY_9}, {0x65, HID_KEY_0},
    {0x27, HID_KEY_Q}, {0x26, HID_KEY_W}, {0x25, HID_KEY_E}, {0x24, HID_KEY_R}, {0x23, HID_KEY_T},
    {0x22, HID_KEY_Y}, {0x21, HID_KEY_U}, {0x76, HID_KEY_I}, {0x75, HID_KEY_O}, {0x64, HID_KEY_P},
    {0x37, HID_KEY_A}, {0x36, HID_KEY_S}, {0x35, HID_KEY_D}, {0x34, HID_KEY_F}, {0x33, HID_KEY_G},
    {0x32, HID_KEY_H}, {0x31, HID_KEY_J}, {0x77, HID_KEY_K}, {0x72, HID_KEY_L}, {0x62, HID_KEY_COMMA},
    {0x46, HID_KEY_Z}, {0x45, HID_KEY_X}, {0x44, HID_KEY_C}, {0x43, HID_KEY_V}, {0x42, HID_KEY_B},
    {0x41, HID_KEY_N}, {0x52, HID_KEY_M}, {0x53, HID_KEY_PERIOD}, {0x63, HID_KEY_ENTER},
    {0x71, HID_KEY_BACKSPACE}, {0x55, HID_KEY_ARROW_LEFT}, {0x54, HID_KEY_SPACE}, {0x51, HID_KEY_ARROW_RIGHT},
};

// The modifier byte's bits
static constexpr uint8_t chatpad_shift = 0x01;
static constexpr uint8_t chatpad_green = 0x02;
static constexpr uint8_t chatpad_orange = 0x04;
static constexpr uint8_t chatpad_people = 0x08;
// Green, orange and people pick a layer or a console feature rather than doing anything on a PC, so they're keys
// a PC ignores when passed through, but which can still be mapped
static constexpr uint8_t chatpad_green_key = HID_KEY_F13;
static constexpr uint8_t chatpad_orange_key = HID_KEY_F14;
static constexpr uint8_t chatpad_people_key = HID_KEY_F15;

// What the green and orange legends on a US chatpad type, as the keys that type them on a US layout. Legends
// with no key of their own there (accented letters, currency signs) type nothing
static constexpr uint16_t with_shift = 0x100;
static const struct
{
    uint8_t scancode;
    uint16_t green;
    uint16_t orange;
} chatpad_layers[] = {
    {0x27, with_shift | HID_KEY_1, 0},                                  // Q: ! ¡
    {0x26, with_shift | HID_KEY_2, 0},                                  // W: @ å
    {0x25, 0, 0},                                                       // E: € é
    {0x24, with_shift | HID_KEY_3, with_shift | HID_KEY_4},             // R: # $
    {0x23, with_shift | HID_KEY_5, 0},                                  // T: % þ
    {0x22, with_shift | HID_KEY_6, 0},                                  // Y: ^ ý
    {0x21, with_shift | HID_KEY_7, 0},                                  // U: & ú
    {0x76, with_shift | HID_KEY_8, 0},                                  // I: * í
    {0x75, with_shift | HID_KEY_9, 0},                                  // O: ( ó
    {0x64, with_shift | HID_KEY_0, HID_KEY_EQUAL},                      // P: ) =
    {0x37, with_shift | HID_KEY_GRAVE, 0},                              // A: ~ á
    {0x36, 0, 0},                                                       // S: š ß
    {0x35, with_shift | HID_KEY_BRACKET_LEFT, 0},                       // D: { ð
    {0x34, with_shift | HID_KEY_BRACKET_RIGHT, 0},                      // F: } £
    {0x33, 0, 0},                                                       // G: ¨ ¥
    {0x32, HID_KEY_SLASH, HID_KEY_BACKSLASH},                           // H: / backslash
    {0x31, HID_KEY_APOSTROPHE, with_shift | HID_KEY_APOSTROPHE},        // J: ' "
    {0x77, HID_KEY_BRACKET_LEFT, 0},                                    // K: [ ¢
    {0x72, HID_KEY_BRACKET_RIGHT, 0},                                   // L: ] ø
    {0x62, with_shift | HID_KEY_SEMICOLON, HID_KEY_SEMICOLON},          // ,: : ;
    {0x46, HID_KEY_GRAVE, 0},                                           // Z: ` æ
    {0x45, 0, 0},                                                       // X: « œ
    {0x44, 0, 0},                                                       // C: » ç
    {0x43, HID_KEY_MINUS, with_shift | HID_KEY_MINUS},                  // V: - _
    {0x42, with_shift | HID_KEY_BACKSLASH, with_shift | HID_KEY_EQUAL}, // B: | +
    {0x41, with_shift | HID_KEY_COMMA, 0},                              // N: < ñ
    {0x52, with_shift | HID_KEY_PERIOD, 0},                             // M: > µ
    {0x53, with_shift | HID_KEY_SLASH, 0},                              // .: ? ¿
};

// What a key types: its key, or with a layer picked, that layer's legend
inline uint16_t chatpad_output(uint8_t scancode, bool green, bool orange, bool shift)
{
    if (green || orange)
    {
        for (const auto &layer : chatpad_layers)
        {
            if (layer.scancode == scancode)
            {
                return green ? layer.green : layer.orange;
            }
        }
    }
    for (const auto &key : chatpad_keycodes)
    {
        if (key[0] == scancode)
        {
            return (shift ? with_shift : 0) | key[1];
        }
    }
    return 0;
}

class Xbox360ChatpadDecoder
{
public:
    // A report: 00, modifiers, key 1, key 2. Returns whether anything is held
    bool handle_report(const uint8_t *buf, uint32_t len)
    {
        if (len < 4 || buf[0] != 0x00)
        {
            return false;
        }
        bool any = false;
        uint8_t modifiers = buf[1] & 0x0F;
        uint8_t pressed_modifiers = modifiers & ~m_last_modifiers;
        m_last_modifiers = modifiers;
        // Shift with orange picked is caps lock, as the console does it
        if ((pressed_modifiers & chatpad_shift) && ((modifiers | m_sticky) & chatpad_orange))
        {
            m_caps = true;
            m_sticky &= ~chatpad_orange;
            pressed_modifiers &= ~chatpad_shift;
        }
        if (!(modifiers & chatpad_shift))
        {
            m_caps = false;
        }
        // Tapping shift, green or orange picks it for the next key, and tapping it again cancels it, also as the
        // console does it. Holding one works too
        m_sticky ^= pressed_modifiers & (chatpad_shift | chatpad_green | chatpad_orange);
        uint8_t active = modifiers | m_sticky;
        // A key types what was picked when it went down, until it's let go
        HeldKey held[2] = {};
        bool typed = false;
        for (uint8_t i = 0; i < 2; i++)
        {
            uint8_t scancode = buf[2 + i];
            if (!scancode)
            {
                continue;
            }
            held[i].scancode = scancode;
            bool found = false;
            for (const auto &previous : m_held)
            {
                if (previous.scancode == scancode)
                {
                    held[i].output = previous.output;
                    found = true;
                }
            }
            if (!found)
            {
                held[i].output = chatpad_output(scancode, active & chatpad_green, active & chatpad_orange, active & chatpad_shift);
                typed = true;
            }
        }
        memcpy(m_held, held, sizeof(held));
        if (typed)
        {
            m_sticky = 0;
        }

        uint32_t keys[8] = {};
        if (m_caps)
        {
            set_key(keys, HID_KEY_CAPS_LOCK);
        }
        else if (modifiers & chatpad_shift)
        {
            set_key(keys, HID_KEY_SHIFT_LEFT);
        }
        if (modifiers & chatpad_green)
        {
            set_key(keys, chatpad_green_key);
        }
        if (modifiers & chatpad_orange)
        {
            set_key(keys, chatpad_orange_key);
        }
        if (modifiers & chatpad_people)
        {
            set_key(keys, chatpad_people_key);
        }
        for (const auto &key : m_held)
        {
            if (key.output & 0xFF)
            {
                set_key(keys, key.output & 0xFF);
            }
            if (key.output & with_shift)
            {
                set_key(keys, HID_KEY_SHIFT_LEFT);
            }
        }
        memcpy(m_keys, keys, sizeof(keys));
        if (modifiers || buf[2] || buf[3])
        {
            any = true;
        }
        return any;
    }
    bool key_pressed(uint8_t keycode) const { return m_keys[keycode >> 5] & (1u << (keycode & 31)); }
    // Modifiers tapped to apply to the next key, which their LEDs show
    uint8_t sticky() const { return m_sticky; }
    void reset()
    {
        memset(m_keys, 0, sizeof(m_keys));
        memset(m_held, 0, sizeof(m_held));
        m_last_modifiers = 0;
        m_sticky = 0;
        m_caps = false;
    }

private:
    static void set_key(uint32_t *keys, uint8_t keycode)
    {
        keys[keycode >> 5] |= 1u << (keycode & 31);
    }
    // The keys held down, and what each types
    struct HeldKey
    {
        uint8_t scancode;
        uint16_t output;
    };
    uint32_t m_keys[8] = {};
    HeldKey m_held[2] = {};
    uint8_t m_last_modifiers = 0;
    uint8_t m_sticky = 0;
    bool m_caps = false;
};
