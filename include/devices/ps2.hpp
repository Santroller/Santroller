#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "input_enums.pb.h"
#include "psx_controller.hpp"
#include <unordered_map>
#include <memory>
#include <set>

class PS2Device : public Device
{
public:
    ~PS2Device() {}
    PS2Device(proto_PSXDevice device, uint16_t id, MultitapPort port);
    void update(bool full_poll, bool send_events);
    uint16_t readAxis(proto_PS2AxisType type);
    bool readButton(proto_PS2ButtonType type);
    bool is_ps2_device(PS2ControllerType type);
    void rescan(bool first);
    bool using_pin(uint8_t pin);

private:
    PSXController m_controller;
    proto_PSXDevice m_device;
    uint32_t m_lastValue = 0;
    PS2ControllerType m_lastControllerType = PS2ControllerType::PS2ControllerTypeUnknown;
    uint8_t m_port;
    uint8_t m_last_seen_ports = 0;
    long m_last_scan = 0;
    std::unordered_map<MultitapPort, std::unique_ptr<PS2Device>> m_devices;
    std::set<MultitapPort> m_seen_devices;
    bool m_has_scanned = false;
};