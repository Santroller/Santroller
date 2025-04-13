#include "state/base.hpp"
#define UP 1 << 0
#define DOWN 1 << 1
#define LEFT 1 << 2
#define RIGHT 1 << 3
static const uint8_t dpad_bindings[] = {0x08, 0x00, 0x04, 0x08, 0x06, 0x07, 0x05, 0x08, 0x02, 0x01, 0x03};
static const uint8_t dpad_bindings_reverse[] = {UP, UP | RIGHT, RIGHT, DOWN | RIGHT, DOWN, DOWN | LEFT, LEFT, UP | LEFT};

class DpadParser {
   public:
    static void updateDpad(uint8_t dpad, san_base_t *data) {
        dpad = dpad >= 0x08 ? 0 : dpad_bindings_reverse[dpad];
        asm volatile("" ::
                         : "memory");
        data->gamepad.dpadUp = dpad & UP;
        data->gamepad.dpadLeft = dpad & LEFT;
        data->gamepad.dpadDown = dpad & DOWN;
        data->gamepad.dpadRight = dpad & RIGHT;
    }
};