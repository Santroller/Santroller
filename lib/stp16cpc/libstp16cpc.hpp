#include "enums.pb.h"
#include "spi.hpp"
class STP16CPC
{
public:
    STP16CPC(uint8_t block, uint8_t mosi, uint8_t sck, uint8_t oe, uint8_t le, uint8_t count);
    void putLeds(uint8_t* leds);

private:
    SPIMasterInterface interface;
    int8_t m_oe;
    int8_t m_le;
    uint8_t m_count;
    uint8_t m_bytes;

};