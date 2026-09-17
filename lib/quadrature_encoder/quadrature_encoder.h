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
    uint sm = 0;
    int old_value = 0;
    uint m_offset = 0;
    bool m_initialized = false;
};