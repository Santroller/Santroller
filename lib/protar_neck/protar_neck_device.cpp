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
    no_attention();
    last = micros();
    lastInit = millis();
}

void ProtarNeck::no_attention(void)
{
    busy_wait_us_32(2);
    gpio_put(m_attPin, true);
}

void ProtarNeck::signal_attention(void)
{
    gpio_put(m_attPin, false);
    busy_wait_us_32(2);
}

uint16_t ProtarNeck::read_axis(ProGuitarNeckAxisType axisType)
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
bool ProtarNeck::read_button(ProGuitarNeckButtonType buttonType)
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
    if (m_state == State::IDLE)
    {
        if (micros() - last > 500)
        {
            last = micros();
            signal_attention();

            uint8_t txBuf[1 + sizeof(protarneck_t)] = {0x80, 0x12, 0x12, 0x12, 0x12, 0x12};
            transferStartMicros = micros();
            if (interface.transfer_dma_start(txBuf, m_rxBuf, sizeof(txBuf)))
            {
                m_state = State::TRANSFERRING;
            }
            else
            {
                // Fallback to blocking transfer if DMA channels unavailable
                interface.transfer(txBuf, m_rxBuf, sizeof(txBuf));
                no_attention();
                uint8_t resp = m_rxBuf[0];
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
                    memcpy(&lastInputs, &m_rxBuf[1], sizeof(lastInputs));
                    lastInput = millis();
                }
            }
        }
    }
    else if (m_state == State::TRANSFERRING)
    {
        if (interface.transfer_dma_busy())
        {
            // DMA timeout check (2ms) to prevent stuck CS/ATT line if hardware stalls
            if (micros() - transferStartMicros > 2000)
            {
                interface.transfer_dma_abort();
                no_attention();
                valid = false;
                m_state = State::IDLE;
            }
        }
        else
        {
            no_attention();
            interface.transfer_dma_finish(m_rxBuf, sizeof(m_rxBuf));

            uint8_t resp = m_rxBuf[0];
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
                memcpy(&lastInputs, &m_rxBuf[1], sizeof(lastInputs));
                lastInput = millis();
            }

            m_state = State::IDLE;
        }
    }

    if (millis() - lastInput > 10)
    {
        memset(&lastInputs, 0, sizeof(lastInputs));
    }
}