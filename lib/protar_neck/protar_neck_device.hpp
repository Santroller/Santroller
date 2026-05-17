#pragma once
#include <stdint.h>

#include "spi.hpp"
#include "enums.pb.h"
#include "input_enums.pb.h"
#include "pico/time.h"
#include <hardware/gpio.h>

typedef struct {
    uint16_t lowEFret : 5;
    uint16_t aFret : 5;
    uint16_t dFret : 5;
    uint16_t : 1; // always 0

    uint16_t gFret : 5;
    uint16_t bFret : 5;
    uint16_t highEFret : 5;
    uint16_t : 1; // always 0

    uint8_t green :1;
    uint8_t red :1;
    uint8_t yellow :1;
    uint8_t blue :1;
    uint8_t orange :1;
    uint8_t soloFlag : 1;
    uint8_t : 1; // always 0
    uint8_t : 1; // always 0
} __attribute__((packed)) protarneck_t;

class ProtarNeck
{
public:
    ProtarNeck(uint8_t block, int8_t sck, int8_t mosi, int8_t miso, uint32_t clock, uint8_t attPin);
    void tick();
    uint16_t readAxis(ProGuitarAxisType type);
    bool readButton(ProGuitarButtonType type);
    void processData(bool ack, bool timeout);

private:
    bool autoShiftData();
    void noAttention();
    void signalAttention();
    SPIMasterInterface interface;
    uint8_t m_attPin;
    bool valid = false;
    long last = 0;
    long lastInit = 0;
    protarneck_t lastInputs;
};