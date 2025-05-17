#include <hardware/gpio.h>
#include <pico/time.h>

#include "stdint.h"
// TODO: if we wanted to be fast it may make sense to use PIO for this in the future
// but if we do want to do that, then we need to keep track of the entire matrix here not just a single set of pins.
class Matrix {
    Matrix(uint8_t pin, uint8_t outPin);
    bool read();

   private:
    uint8_t pin;
    uint8_t outPin;
};