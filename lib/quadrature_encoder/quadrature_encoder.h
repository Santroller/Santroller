void quad_init();
int quad_tick();
#include <stdint.h>
#include <hardware/pio.h>
class QuadratureEncoder
{
public:
    QuadratureEncoder(uint8_t pin, uint8_t divisor = 1);
    void tick();
    void begin();
    void end();
    int delta = 0;
    int position = 0;

private:
    uint8_t m_pin;
    uint8_t m_divisor = 1;
    int m_sub_count = 0;
    PIO pio = pio1;
    uint sm = 0;
    int old_value = 0;
    uint m_offset = 0;
    bool m_initialized = false;
};