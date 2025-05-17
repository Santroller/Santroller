#include "guitar_hero_5_neck.hpp"
void GuitarHero5Neck::tick() {
    uint8_t gh5Data[2];
    connected = interface->readRegister(GH5NECK_ADDR, GH5NECK_BUTTONS_PTR, sizeof(gh5Data), gh5Data);
    if (connected) {
        green = gh5Data[0] & 1 << 4;
        red = gh5Data[0] & 1 << 5;
        yellow = gh5Data[0] & 1 << 6;
        blue = gh5Data[0] & 1 << 7;
        orange = gh5Data[0] & 1 << 0;
        slider = gh5Data[1];
    }
};