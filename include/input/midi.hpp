#pragma once
#include "input.hpp"
#include "input.pb.h"
#include "devices/base.hpp"
#include <memory>
class MidiNoteInput : public Input
{
public:
    MidiNoteInput(proto_MidiNoteInput input, std::shared_ptr<MidiDevice> device);
    bool tickDigital();
    uint16_t tickAnalog();

private:
    void setup();
    proto_MidiNoteInput m_input;
    std::shared_ptr<MidiDevice> m_device;
};
class MidiControlChangeInput : public Input
{
public:
    MidiControlChangeInput(proto_MidiControlChangeInput input, std::shared_ptr<MidiDevice> device);
    bool tickDigital();
    uint16_t tickAnalog();

private:
    void setup();
    proto_MidiControlChangeInput m_input;
    std::shared_ptr<MidiDevice> m_device;
};
class MidiPitchBendInput : public Input
{
public:
    MidiPitchBendInput(proto_MidiPitchBendInput input, std::shared_ptr<MidiDevice> device);
    bool tickDigital();
    uint16_t tickAnalog();

private:
    void setup();
    proto_MidiPitchBendInput m_input;
    std::shared_ptr<MidiDevice> m_device;
};