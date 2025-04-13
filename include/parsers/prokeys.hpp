#pragma once
#include "state/base.hpp"
#define PRO_KEY_COUNT 25
#define PRO_KEY_FIRST_NOTE 24
#define DEFAULT_VELOCITY 64
class ProKeysParser {
   public:
    template <typename T>
    static void updateProKeys(T* report, san_base_t* data, uint8_t pedalAnalog) {
        data->gamepad.a = report->a;
        data->gamepad.b = report->b;
        data->gamepad.x = report->x;
        data->gamepad.y = report->y;
        data->gamepad.back = report->back;
        data->gamepad.start = report->start;
        data->gamepad.guide = report->guide;
        if (report->pedalDigital) {
            data->midi.midiSustainPedal = 0x7f;
        } else if (pedalAnalog) {
            data->midi.midiSustainPedal = pedalAnalog;
        } else {
            data->midi.midiSustainPedal = 0;
        }
        data->midi.midiModWheel = report->overdrive ? 0xFF : 0x00;
        /* uint8 -> int14 (but only the positive half!) */
        data->midi.midiPitchWheel = report->touchPad << 5;
        uint32_t keyMask =
            (report->key1 << 17) |
            (report->key2 << 9) |
            (report->key3 << 1) |
            ((report->velocities[0] & 0x80) >> 7);

        int pressed = 0;
        for (int i = 0; i < PRO_KEY_COUNT; i++) {
            int keyBit = 1 << (KEY_COUNT - 1 - i);
            if (keyMask & keyBit) {
                if (pressed < 5) {
                    data->midi.midiVelocities[PRO_KEY_FIRST_NOTE + i] = (report->velocities[pressed++] & 0x7F);
                } else {
                    data->midi.midiVelocities[PRO_KEY_FIRST_NOTE + i] = DEFAULT_VELOCITY;
                }
            } else {
                data->midi.midiVelocities[PRO_KEY_FIRST_NOTE + i] = 0;
            }
        }
    }
};