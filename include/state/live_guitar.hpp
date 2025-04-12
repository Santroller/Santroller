#include <stdint.h>
#include "gamepad.hpp"
typedef struct {
    san_gamepad_t gamepad;
    uint8_t black1 : 1;
    uint8_t black2 : 1;
    uint8_t black3 : 1;
    uint8_t white1 : 1;
    uint8_t white2 : 1;
    uint8_t white3 : 1;
    uint8_t strumUp : 1;
    uint8_t strumDown : 1;
    uint8_t heroPower : 1;
    uint8_t ghtv : 1;
    uint8_t : 6;
    uint8_t whammy;
    uint8_t tilt;
} san_live_guitar_t;