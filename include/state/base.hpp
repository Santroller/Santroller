#pragma once
#include "gamepad.hpp"
#include "guitar.hpp"
#include "keyboard_mouse.hpp"
#include "live_guitar.hpp"
#include "midi.hpp"
#include "pro_guitar.hpp"
#include "taiko.hpp"
#include "turntable.hpp"
#include "wheel.hpp"

#define PS3_STICK_CENTER 0x80
#define PS3_ACCEL_CENTER 0x0200

typedef struct {
    san_gamepad_t gamepad;
    san_gamepad_pressures_t gamepad_pressures;
    san_gh_rb_guitar_t guitar;
    san_keyboard_t keyboard;
    san_mouse_t mouse;
    san_live_guitar_t live_guitar;
    san_midi_t midi;
    san_pro_guitar_t pro_guitar;
    san_turntable_t turntable;
    san_taiko_t taiko;
    san_wheel_t wheel;
} san_base_t;