#include "dj_hero_turntable.hpp"
void DJHeroTurntable::tick()
{
    uint8_t start[2] = {0};
    uint8_t data[3];
    // TODO: need to get a table out and see what happens when its polled fast in this manner
    connected = interface.readRegisterRepeatedStart(addr, DJ_BUTTONS_PTR, sizeof(start), start);
    if (connected && start[1])
    {
        connected = interface.readFrom(addr, data, sizeof(data), true);
        velocity = (int8_t)data[2];
        green = data[0] & (1 << 4);
        red = data[0] & (1 << 5);
        blue = data[0] & (1 << 6);
    }
}