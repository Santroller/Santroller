#pragma once
#include <MIDI.h>

#include <SimpleMidiTransport.hpp>

#include "i2c.hpp"
#include "base.hpp"
#include "device.pb.h"
#include "libmpr121.hpp"
#pragma once
enum USBDeviceType
{
    Santroller,
    XInput,
    PS3,
    PS4,
    PS5,
    Raphnet,
    Switch,
    XboxOne,
    OGXbox
};

class USBDevice : public MidiDevice
{
public:
    ~USBDevice() {}
    USBDevice(proto_UsbHostDevice device, uint16_t id);
    void update(bool full_poll);
    bool using_pin(uint8_t pin);
    uint16_t readMidiNote(uint8_t note);
    uint16_t readMidiControlChange(uint8_t cc);
    int16_t readMidiPitchBend();

private:
    proto_UsbHostDevice m_device;
    MIDI_NAMESPACE::SimpleMidiInterface midiInterface;
    uint8_t mReportData[128];
    // Parser &mParser;
};