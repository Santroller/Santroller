#include <stdint.h>
#include "gamepad.hpp"
typedef struct {
    san_gamepad_t gamepad;
    uint8_t leftGreen : 1;
    uint8_t leftRed : 1;
    uint8_t leftBlue : 1;

    uint8_t rightGreen : 1;
    uint8_t rightRed : 1;
    uint8_t rightBlue : 1;

    uint8_t euphoria : 1;
    uint8_t : 1;

    uint8_t leftTableVelocity;
    uint8_t rightTableVelocity;

    uint8_t effectsKnob;
    uint8_t crossfader;
} san_turntable_t;