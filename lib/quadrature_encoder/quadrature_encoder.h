void quad_init();
int quad_tick();
#include <stdint.h>
#include <hardware/pio.h>
class QuadratureEncoder
{
public:
    QuadratureEncoder(uint8_t pin);
    void tick();
    int delta;

private:
    uint8_t m_pin;
    PIO pio = pio1;
    const uint sm = 0;
    int old_value = 0;
};