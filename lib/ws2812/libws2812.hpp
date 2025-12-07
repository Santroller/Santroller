#include "ws2812.pio.h"
#include "enums.pb.h"
class WS2812
{
public:
    WS2812(uint8_t pin, WS2812Type type);
    void putLed(uint8_t a, uint8_t b, uint8_t c);

private:
    uint8_t pin;
    bool hasW;
    WS2812Type m_type;
    PIO ws2812Pio;
    uint ws2812Sm;
    uint ws2812Offset;
};