#pragma once
#include <stdint.h>
#include <memory>
#include "device.pb.h"
#include "instance.hpp"
#include "protocols/nintendo.hpp"

class SnesEmulation;

// Builds SNES / NES button states from a profile's mappings
class SNESEmulationDeviceInstance : public Instance
{
public:
    ~SNESEmulationDeviceInstance();
    SNESEmulationDeviceInstance(proto_SNESEmulationDevice device);
    void initialize();
    void process(bool full_poll, bool send_events);

private:
    proto_SNESEmulationDevice m_device;
    std::weak_ptr<class SNESEmulationDevice> m_snes_dev;
    SnesEmulation *m_controller = nullptr;
    bool m_acquired = false;
    bool m_nes = false;
};
