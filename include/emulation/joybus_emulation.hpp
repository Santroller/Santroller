#pragma once
#include <stdint.h>
#include <memory>
#include "device.pb.h"
#include "instance.hpp"
#include "protocols/nintendo.hpp"

class JoybusEmulation;

// Builds GameCube / N64 reports from a profile's mappings
class JoybusEmulationDeviceInstance : public Instance
{
public:
    ~JoybusEmulationDeviceInstance();
    JoybusEmulationDeviceInstance(proto_JoybusEmulationDevice device);
    void initialize();
    void process(bool full_poll, bool send_events);

private:
    proto_JoybusEmulationDevice m_device;
    std::weak_ptr<class JoybusEmulationDevice> m_joybus_dev;
    JoybusEmulation *m_controller = nullptr;
    bool m_acquired = false;
    bool m_n64 = false;
    uint8_t m_initial_report[sizeof(GameCubeGamepad_Data_t)];
    uint8_t m_buffer[sizeof(GameCubeGamepad_Data_t)];
};
