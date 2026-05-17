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
    case ProGuitarLowEFret:
        return lastInputs.lowEFret;
    case ProGuitarAFret:
        return lastInputs.aFret;
    case ProGuitarDFret:
        return lastInputs.dFret;
    case ProGuitarGFret:
        return lastInputs.gFret;
    case ProGuitarBFret:
        return lastInputs.bFret;
    case ProGuitarHighEFret:
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
    case ProGuitarGreen:
        return lastInputs.green;
    case ProGuitarRed:
        return lastInputs.red;
    case ProGuitarYellow:
        return lastInputs.yellow;
    case ProGuitarBlue:
        return lastInputs.blue;
    case ProGuitarOrange:
        return lastInputs.orange;
    case ProGuitarSoloGreen:
        return lastInputs.green && lastInputs.soloFlag;
    case ProGuitarSoloRed:
        return lastInputs.red && lastInputs.soloFlag;
    case ProGuitarSoloYellow:
        return lastInputs.yellow && lastInputs.soloFlag;
    case ProGuitarSoloBlue:
        return lastInputs.blue && lastInputs.soloFlag;
    case ProGuitarSoloOrange:
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
            memset(&lastInputs, 0, sizeof(lastInputs));
            noAttention();
            return;
        }
        if (resp != 0x80)
        {
            valid = false;
            noAttention();
            return;
        }
        valid = true;
        uint8_t *buf = (uint8_t *)&lastInputs;
        for (size_t i = 0; i < sizeof(lastInputs); i++)
        {
            buf[i] = interface.transfer(0x12);
        }
        noAttention();
    }
}