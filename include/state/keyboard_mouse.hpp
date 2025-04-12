#include <stdint.h>
#include "gamepad.hpp"
#define SIMULTANEOUS_KEYS 10

// TODO: does 10kro work on PS? IF not, go back to 6KRO
typedef struct {
    bool leftCtrl : 1;
    bool leftShift : 1;
    bool leftAlt : 1;
    bool lWin : 1;
    bool rightCtrl : 1;
    bool rightShift : 1;
    bool rightAlt : 1;
    bool rWin : 1;
    uint8_t KeyCode[SIMULTANEOUS_KEYS];
} san_keyboard_t;
typedef struct {
    bool left : 1;
    bool right : 1;
    bool middle : 1;
    uint8_t : 5;
    int8_t x;
    int8_t y;
    int8_t scrollY;
    int8_t scrollX;
} san_mouse_t;