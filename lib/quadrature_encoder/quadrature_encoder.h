void quad_init();
int quad_tick();
#include <stdint.h>
#include <hardware/pio.h>
class QuadratureEncoder
{
public:
    QuadratureEncoder(uint8_t pin);
    void tick();
    void begin();
    void end();
    int delta;
    int position;

private:
    uint8_t m_pin;
    PIO pio = pio1;
    const uint sm = 0;
    uint16_t old_value = 0;
    int m_offset = 0;
};