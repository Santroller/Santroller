#include "libapa102.hpp"
#include "stdio.h"
APA102::APA102(uint8_t block, int8_t mosi, int8_t sck, uint8_t count, APA102Type type):interface(block, SPI_CPHA_1, SPI_CPOL_1, sck, mosi, -1, true, 12000000), m_count(count), m_type(type) {
    
}
void APA102::putLed(uint8_t brightness, uint8_t r, uint8_t g, uint8_t b) {
    uint8_t x = r, y = g, z = b;
    switch (m_type)
    {
    case Apa102Rgb:
        x = r;
        y = g;
        z = b;
        break;
    case Apa102Rbg:
        x = r;
        y = b;
        z = g;
        break;
    case Apa102Grb:
        x = g;
        y = r;
        z = b;
        break;
    case Apa102Gbr:
        x = g;
        y = b;
        z = r;
        break;
    case Apa102Brg:
        x = b;
        y = r;
        z = g;
        break;
    case Apa102Bgr:
        x = b;
        y = g;
        z = r;
        break;
    }
    interface.transfer(brightness | 0xE0);
    interface.transfer(x);
    interface.transfer(y);
    interface.transfer(z);
}
void APA102::begin() {
    interface.transfer(0x00);
    interface.transfer(0x00);
    interface.transfer(0x00);
    interface.transfer(0x00);
}
void APA102::end() {
    for (size_t i = 0; i < (m_count + 14)/16; i++)
    {
        interface.transfer(0x00);
    }
}