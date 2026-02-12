#include "ws2812.pio.h"
#include "enums.pb.h"
class WS2812
{
public:
    WS2812(uint8_t pin, uint8_t count, WS2812Type type);
    ~WS2812();
    void putLeds(uint32_t *leds, uint8_t count);

private:
    uint8_t m_pin;
    bool hasW;
    WS2812Type m_type;
    PIO ws2812Pio;
    uint ws2812Sm;
    uint ws2812Offset;
    uint32_t led_state[255];
};