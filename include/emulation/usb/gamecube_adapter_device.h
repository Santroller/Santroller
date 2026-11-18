#pragma once
#include "hid_device.h"
#include "protocols/gamecube_adapter.hpp"
#include "protocols/nintendo.hpp"

// Nintendo's Wii U GameCube adapter (WUP-028). The Switch, the Wii U, Dolphin and SDL all take it as
// GameCube controllers. Only used when a profile forces ModeGameCubeAdapter, as nothing about a host says it
// wants one. There is only ever one, and each profile on it is a port, in the order they were added.
class GameCubeAdapterDevice : public HIDDevice
{
public:
    void initialize() override;
    void process(bool full_poll, bool send_events) override;
    size_t compatible_section_descriptor(uint8_t *desc, size_t remaining) override;
    size_t config_descriptor(uint8_t *desc, size_t remaining) override;
    size_t device_name(uint8_t idx, char *desc) override;
    void device_descriptor(tusb_desc_device_t *desc) override;
    const uint8_t *report_descriptor() override;
    uint16_t report_desc_len() override;
    uint16_t get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) override;
    void set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) override;
    void profile_rumble(size_t index, uint8_t &left, uint8_t &right) const override;

private:
    bool winusb() const;
    // Like the real adapter, nothing is sent until the host starts it with GAMECUBE_ADAPTER_INIT
    bool m_started = false;
    uint8_t m_port_rumble[GAMECUBE_ADAPTER_PORTS] = {};
    uint8_t m_report[GAMECUBE_ADAPTER_INPUT_REPORT_LEN] = {};
};
