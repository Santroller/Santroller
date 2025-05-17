#include "dj_hero_turntable.hpp"
void DJHeroTurntable::tick() {
    uint8_t data[3];
    connected = interface->readRegister(DJLEFT_ADDR, DJ_BUTTONS_PTR, sizeof(data), data);
    if (connected) {
        velocity = (int8_t)data[2];
        green = data[0] & (1 << 4);
        red = data[0] & (1 << 5);
        blue = data[0] & (1 << 6);
    }
}