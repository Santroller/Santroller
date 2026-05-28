#include <hardware/gpio.h>
#include <pico/time.h>

#include "stdint.h"
class Matrix
{
public:
    Matrix(uint32_t pins, uint32_t outPins);
    bool read(uint32_t outPins, uint8_t in_pin);

private:
    uint32_t m_pins;
    uint32_t m_outPins;
};