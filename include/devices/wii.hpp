#pragma once
#include "base.hpp"
#include "midi.hpp"
#include "device.pb.h"
#include "input_enums.pb.h"
#include "wii_extension.hpp"
class WiiDevice : public MidiDevice
{
public:
    WiiDevice(const DeviceReloadState* state, proto_WiiDevice device, uint16_t id);
    ~WiiDevice();
    void begin();
    void end(bool full);
    void rescan(bool first);
    void update(bool full_poll, bool send_events);
    uint16_t read_axis(proto_WiiAxisType type);
    bool read_button(proto_WiiButtonType type);
    bool is_wii_extension(WiiExtType type);
    bool using_pin(uint8_t pin);
    void save_reload_state(DeviceReloadState& state) const override;

private:
    WiiExtension m_extension;
    proto_WiiDevice m_device;
    uint32_t m_last_value = 0;
    WiiExtType m_lastExtType = WiiExtType::WiiNoExtension;
    bool m_has_scanned = false;
};