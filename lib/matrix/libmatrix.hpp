#include <hardware/gpio.h>
#include <pico/time.h>

#include "stdint.h"
class Matrix
{
public:
    Matrix(uint64_t pins, uint64_t outPins, bool active_low = false);
    bool read(uint8_t out_pin, uint8_t in_pin);

private:
    uint64_t m_pins;
    uint64_t m_outPins;
    bool m_active_low;
};