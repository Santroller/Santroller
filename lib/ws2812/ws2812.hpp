#include "ws2812.pio.h"
class WS2812 {
    WS2812(uint8_t pin, bool hasW);
    void putLed(uint8_t a, uint8_t b, uint8_t c);

   private:
    bool hasW;
    uint8_t pin;
    PIO ws2812Pio;
    uint ws2812Sm;
    uint ws2812Offset;
};