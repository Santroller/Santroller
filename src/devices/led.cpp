#include "devices/apa102.hpp"
#include "events.pb.h"
#include "main.hpp"
void LedDevice::set_led(uint8_t i, uint8_t r, uint8_t g, uint8_t b)
{
    led_state[i] = (r) | (g << 8) | (b << 16);
}