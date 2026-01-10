#include "guitar_hero_5_neck.hpp"
#include "stdio.h"
#include "utils.h"

void GuitarHero5Neck::tick() {
    uint8_t start;
    uint8_t gh5Data[4];
    connected = interface.readRegisterRepeatedStart(GH5NECK_ADDR, GH5NECK_BUTTONS_PTR, 1, &start);
    if (connected && start) {
        connected = interface.readFrom(GH5NECK_ADDR, gh5Data, sizeof(gh5Data), true);
        green = gh5Data[0] & 1 << 4;
        red = gh5Data[0] & 1 << 5;
        yellow = gh5Data[0] & 1 << 6;
        blue = gh5Data[0] & 1 << 7;
        orange = gh5Data[0] & 1 << 0;
        tapGreen = gh5Data[3] & 1 << 4;
        tapRed = gh5Data[3] & 1 << 3;
        tapYellow = gh5Data[3] & 1 << 2;
        tapBlue = gh5Data[3] & 1 << 1;
        tapOrange = gh5Data[3] & 1 << 0;
    }
};