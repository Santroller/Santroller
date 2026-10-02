#include "libapa102.hpp"
#include <stdio.h>
#include <string.h>

APA102::APA102(uint8_t block, int8_t mosi, int8_t sck, uint16_t count, APA102Type type)
    : interface(block, SPI_CPHA_1, SPI_CPOL_1, sck, mosi, -1, true, 12000000), m_count(count), m_type(type)
{
    size_t endBytes = (count + 14) / 16;
    m_buf_len = 4 + (size_t)count * 4 + endBytes;
    m_buf = new uint8_t[m_buf_len];
    memset(m_buf, 0, m_buf_len);
}

APA102::~APA102()
{
    if (interface.transfer_dma_busy()) {
        interface.transfer_dma_finish(nullptr, 0);
    }
    if (m_buf) {
        delete[] m_buf;
        m_buf = nullptr;
    }
}

void APA102::putLeds(const uint32_t *leds, uint16_t count)
{
    if (!m_buf || count == 0) return;

    if (interface.transfer_dma_busy()) {
        interface.transfer_dma_finish(nullptr, 0);
    }

    size_t ledsToUpdate = count > m_count ? m_count : count;
    for (size_t i = 0; i < ledsToUpdate; i++)
    {
        uint32_t color = leds[i];
        uint8_t r = color & 0xff;
        uint8_t g = (color >> 8) & 0xff;
        uint8_t b = (color >> 16) & 0xff;
        uint8_t brightness = (color >> 24) & 0xff;

        uint8_t x = r, y = g, z = b;
        switch (m_type)
        {
        case Apa102Rgb: x = r; y = g; z = b; break;
        case Apa102Rbg: x = r; y = b; z = g; break;
        case Apa102Grb: x = g; y = r; z = b; break;
        case Apa102Gbr: x = g; y = b; z = r; break;
        case Apa102Brg: x = b; y = r; z = g; break;
        case Apa102Bgr: x = b; y = g; z = r; break;
        }

        size_t idx = 4 + i * 4;
        m_buf[idx + 0] = (uint8_t)(((brightness >> 3) & 0x1f) | 0xE0);
        m_buf[idx + 1] = x;
        m_buf[idx + 2] = y;
        m_buf[idx + 3] = z;
    }

    interface.transfer_dma_start(m_buf, nullptr, m_buf_len);
}

void APA102::putLed(uint8_t brightness, uint8_t r, uint8_t g, uint8_t b) {
    uint8_t x = r, y = g, z = b;
    switch (m_type)
    {
    case Apa102Rgb: x = r; y = g; z = b; break;
    case Apa102Rbg: x = r; y = b; z = g; break;
    case Apa102Grb: x = g; y = r; z = b; break;
    case Apa102Gbr: x = g; y = b; z = r; break;
    case Apa102Brg: x = b; y = r; z = g; break;
    case Apa102Bgr: x = b; y = g; z = r; break;
    }
    uint8_t packet[4] = {
        (uint8_t)(((brightness >> 3) & 0x1f) | 0xE0),
        x, y, z
    };
    interface.transfer(packet, nullptr, 4);
}

void APA102::begin() {
    uint8_t startFrame[4] = {0x00, 0x00, 0x00, 0x00};
    interface.transfer(startFrame, nullptr, 4);
}

void APA102::end() {
    size_t endBytes = (m_count + 14) / 16;
    if (endBytes > 0)
    {
        uint8_t endFrame[32] = {0};
        size_t len = endBytes > sizeof(endFrame) ? sizeof(endFrame) : endBytes;
        interface.transfer(endFrame, nullptr, len);
    }
}