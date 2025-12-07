#include "spi.hpp"
#include "enums.pb.h"
class APA102
{
public:
    APA102(uint8_t block, int8_t mosi, int8_t sck, uint8_t count, APA102Type type);
    void putLed(uint8_t brightness, uint8_t r, uint8_t g, uint8_t b);
    void begin();
    void end();

private:
    SPIMasterInterface interface;
    uint8_t m_count;
    APA102Type m_type;
};