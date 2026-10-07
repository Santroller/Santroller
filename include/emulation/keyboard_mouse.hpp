#pragma once
#include <stdint.h>
#include <memory>
#include <vector>
#include "profiles/profile.hpp"

// The keyboard and mouse share one HID interface (USB) / service (BLE), told apart by report id
#define KEYBOARD_REPORT_ID 1
#define MOUSE_REPORT_ID 2

// The standard boot keyboard / mouse layouts. Defined here rather than taken from tinyusb,
// as its hid.h clashes with btstack's in the BLE code.
typedef struct __attribute__((packed))
{
    uint8_t modifier;
    uint8_t reserved;
    uint8_t keycode[6];
} KeyboardReport;

typedef struct __attribute__((packed))
{
    uint8_t buttons;
    int8_t x;
    int8_t y;
    int8_t wheel;
    int8_t pan;
} MouseReport;

// Turns the keyboard / mouse state the mappings filled in into reports, shared by the USB and
// BLE keyboard devices. Keys take priority, the mouse goes out once the keyboard is up to date.
class KeyboardMouseReports
{
public:
    void reset();
    // Call after the profiles' mappings have run
    void collect(const std::vector<std::shared_ptr<Profile>> &profiles);
    // The keyboard report, if it changed since it was last sent
    bool keyboard_pending(KeyboardReport &report) const;
    void keyboard_sent(const KeyboardReport &report);
    // A mouse report, if there's movement or a button change to send
    bool mouse_pending(MouseReport &report) const;
    void mouse_sent(const MouseReport &report);

private:
    KeyboardReport m_keyboard = {};
    KeyboardReport m_last_keyboard = {};
    bool m_sent_keyboard = false;
    uint8_t m_mouse_buttons = 0;
    uint8_t m_last_mouse_buttons = 0;
    // mouse movement not sent yet, in deflection * us * speed units
    int64_t m_mouse_acc[MouseState::AxisCount] = {0};
    uint32_t m_last_mouse_us = 0;
};
