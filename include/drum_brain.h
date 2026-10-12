#pragma once
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
// Pads reported by the Guitar Hero World Tour and Band Hero drum brains
#define DRUM_BRAIN_KICK 0
#define DRUM_BRAIN_RED 1
#define DRUM_BRAIN_YELLOW 2
#define DRUM_BRAIN_BLUE 3
#define DRUM_BRAIN_ORANGE 4
#define DRUM_BRAIN_GREEN 5
#define DRUM_BRAIN_PAD_COUNT 6
extern uint8_t drum_brain_velocities[DRUM_BRAIN_PAD_COUNT];
extern bool drum_brain_found;
void tick_wt_drum(void);
void tick_bh_drum(void);
#ifdef __cplusplus
}
#endif
