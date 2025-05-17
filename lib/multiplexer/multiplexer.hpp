#include <hardware/gpio.h>
#include <hardware/adc.h>
#include <pico/time.h>

#include "stdint.h"
class Multiplexer {
    Multiplexer(uint8_t s0, uint8_t s1, uint8_t s2, uint8_t input);
    Multiplexer(uint8_t s0, uint8_t s1, uint8_t s2, uint8_t s3, uint8_t input);
    uint16_t read(uint8_t channel);
    uint8_t s0Pin;
    uint8_t s1Pin;
    uint8_t s2Pin;
    uint8_t s3Pin;
    uint8_t inputPin;
    bool sixteenChannel;
};