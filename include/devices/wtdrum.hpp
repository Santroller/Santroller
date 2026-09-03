#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "world_tour_drum.hpp"
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
    uint32_t m_last_value = 0;
};