#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "input_enums.pb.h"
#include "wii_extension.hpp"
class WiiDevice : public MidiDevice
{
public:
    ~WiiDevice() {}
    WiiDevice(proto_WiiDevice device, uint16_t id);
    void rescan(bool first);
    void update(bool full_poll, bool send_events);
    uint16_t readAxis(proto_WiiAxisType type);
    bool readButton(proto_WiiButtonType type);
    bool is_wii_extension(WiiExtType type);
    uint16_t readMidiNote(uint8_t note);
    uint16_t readMidiControlChange(uint8_t cc);
    int16_t readMidiPitchBend();
    bool using_pin(uint8_t pin);

private:
    WiiExtension m_extension;
    proto_WiiDevice m_device;
    uint32_t m_lastValue = 0;
    WiiExtType m_lastExtType = WiiExtType::WiiNoExtension;
    bool m_has_scanned = false;
};