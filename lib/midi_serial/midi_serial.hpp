#pragma once

#include <stdint.h>
#include "uart.hpp"
#include "devices/midi.hpp"

class MidiSerial
{
public:
    MidiSerial(MidiDevice* midiDevice, uint8_t block, uint8_t tx, uint8_t rx, uint32_t clock);
    ~MidiSerial();
    void tick();
    inline bool is_connected()
    {
        return true;
    }

private:
    UARTInterface interface;
    MidiDevice *m_device;
};
