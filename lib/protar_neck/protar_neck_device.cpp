#include "protar_neck_device.hpp"
#include <hardware/gpio.h>
#include <pico/time.h>
#include <stdio.h>
#include "utils.h"

ProtarNeck::ProtarNeck(uint8_t block, int8_t sck, int8_t mosi, int8_t miso, uint32_t clock, uint8_t attPin) : interface(block, SPI_CPHA_1, SPI_CPOL_0, sck, mosi, miso, false, clock), m_attPin(attPin)
{
    printf("protar neck init!\r\n");
    gpio_init(attPin);
    gpio_set_dir(attPin, true);
    gpio_set_pulls(attPin, false, false);
    last = micros();
    lastInit = millis();
}

void ProtarNeck::noAttention(void)
{
    gpio_put(m_attPin, true);
}
void ProtarNeck::signalAttention(void)
{
    gpio_put(m_attPin, false);
}

uint16_t ProtarNeck::readAxis(ProGuitarNeckAxisType axisType)
{
    switch (axisType)
    {
    case ProGuitarNeckLowEFret:
        return lastInputs.lowEFret;
    case ProGuitarNeckAFret:
        return lastInputs.aFret;
    case ProGuitarNeckDFret:
        return lastInputs.dFret;
    case ProGuitarNeckGFret:
        return lastInputs.gFret;
    case ProGuitarNeckBFret:
        return lastInputs.bFret;
    case ProGuitarNeckHighEFret:
        return lastInputs.highEFret;
    default:
        return 0;
    }
    return 0;
}
bool ProtarNeck::readButton(ProGuitarNeckButtonType buttonType)
{

    switch (buttonType)
    {
    case ProGuitarNeckGreen:
        return lastInputs.green;
    case ProGuitarNeckRed:
        return lastInputs.red;
    case ProGuitarNeckYellow:
        return lastInputs.yellow;
    case ProGuitarNeckBlue:
        return lastInputs.blue;
    case ProGuitarNeckOrange:
        return lastInputs.orange;
    case ProGuitarNeckSoloGreen:
        return lastInputs.green && lastInputs.soloFlag;
    case ProGuitarNeckSoloRed:
        return lastInputs.red && lastInputs.soloFlag;
    case ProGuitarNeckSoloYellow:
        return lastInputs.yellow && lastInputs.soloFlag;
    case ProGuitarNeckSoloBlue:
        return lastInputs.blue && lastInputs.soloFlag;
    case ProGuitarNeckSoloOrange:
        return lastInputs.orange && lastInputs.soloFlag;
    default:
        return 0;
    }
    return false;
}
void ProtarNeck::tick()
{
    if (micros() - last > 500)
    {
        last = micros();
        signalAttention();
        uint8_t resp = interface.transfer(0x80);
        if (resp == 0x00)
        {
            valid = true;
        }
        else if (resp != 0x80)
        {
            valid = false;
        }
        else
        {
            valid = true;
            uint8_t *buf = (uint8_t *)&lastInputs;
            for (size_t i = 0; i < sizeof(lastInputs); i++)
            {
                buf[i] = interface.transfer(0x12);
            }
            lastInput = millis();
        }
    }
    noAttention();
    if (millis() - lastInput > 10)
    {
        memset(&lastInputs, 0, sizeof(lastInputs));
    }
}