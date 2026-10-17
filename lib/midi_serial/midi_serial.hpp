#pragma once

#include <stdint.h>
#include "uart.hpp"
#include "devices/midi.hpp"

class MidiSerial
{
public:
    MidiSerial(MidiDevice* midiDevice, uint8_t block, int8_t tx, int8_t rx, uint32_t clock);
    ~MidiSerial();
    void tick();
    bool writable() { return interface.writable(); }
    void put(uint8_t data) { interface.put(data); }
    inline bool is_connected()
    {
        return true;
    }

private:
    UARTInterface interface;
    MidiDevice *m_device;
};
