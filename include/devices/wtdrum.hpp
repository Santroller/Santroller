#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "world_tour_drum.hpp"
#include "midi.hpp"
class WorldTourDrumDevice : public MidiDevice
{
public:
    ~WorldTourDrumDevice() {}
    WorldTourDrumDevice(const DeviceReloadState* state, proto_WorldTourDrumDevice device, uint16_t id);
    void begin();
    void end(bool full);
    void update(bool full_poll, bool send_events);
    bool using_pin(uint8_t pin);

private:
    WorldTourDrum m_world_tour_drum;
    proto_WorldTourDrumDevice m_device;
    // Room for a whole read from the drum
    MidiStaticBuffers<64, 0, 32, 1> m_midi_buffers;
};