#pragma once
#include "spi.hpp"
#include "enums.pb.h"
#include <stdint.h>
#include <stddef.h>

class APA102
{
public:
    APA102(uint8_t block, int8_t mosi, int8_t sck, uint16_t count, APA102Type type);
    ~APA102();
    void putLeds(const uint32_t *leds, uint16_t count);
    void putLed(uint8_t brightness, uint8_t r, uint8_t g, uint8_t b);
    void begin();
    void end();

private:
    SPIMasterInterface interface;
    uint16_t m_count;
    APA102Type m_type;
    uint8_t *m_buf = nullptr;
    size_t m_buf_len = 0;
};