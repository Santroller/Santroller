#pragma once
#include <stdint.h>
typedef struct {
    uint8_t leftDrumRim : 1;
    uint8_t leftDrumCenter : 1;
    uint8_t rightDrumRim : 1;
    uint8_t rightDrumCenter : 1;
} san_taiko_t;
