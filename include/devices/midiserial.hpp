#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "midi_serial.hpp"
#include "midi.hpp"
class MidiSerialDevice : public MidiDevice
{
public:
    ~MidiSerialDevice() {}
    MidiSerialDevice(proto_SerialMidiDevice device, uint16_t id);
    void update(bool full_poll, bool send_events);
    bool using_pin(uint8_t pin);
    MidiSerial serial;

private:
    proto_SerialMidiDevice m_device;
};