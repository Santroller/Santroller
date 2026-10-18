#pragma once
#include "devices/usb/host/host.hpp"
#include "protocols/gamecube_adapter.hpp"

// Virtual interface numbers for the ports, kept clear of the adapter's real interfaces. EVORETRO adapters
// with a port per interface get theirs from the interface they are on
#define GAMECUBE_ADAPTER_PORT_INTERFACE(itf, port) (0x80 | ((itf) << 2) | (port))

// A port of an adapter, assignable while a controller is plugged into it
class GameCubePortHost : public UsbHostInterface
{
public:
    GameCubePortHost(uint8_t port, uint8_t dev_addr, uint8_t adapter_itf, uint16_t id, uint16_t vid, uint16_t pid);
    // the adapter does all the transfers
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes) override { return false; }
    bool set_config() override { return true; }
    bool tick_digital(proto_Output &type) override;
    uint16_t tick_analog(proto_Output &type) override;
    void set_rumble(uint8_t left, uint8_t right) override { m_rumble = left || right; }
    bool has_rumble() const override { return true; }
    bool rumble() const { return m_rumble; }
    // Name it after the port it is on, for the config tool
    void set_port_name(uint8_t number);
    // Written by the adapter. Kept here rather than read from the adapter, since a profile can hold on to a
    // port after the adapter is unplugged
    GameCubePortState state;
    GameCubeAxisRange range;

private:
    bool m_rumble = false;
};

// The adapter's interface. It isn't a controller itself, its ports are
class GameCubeAdapterHost : public UsbHostInterface
{
public:
    GameCubeAdapterHost(uint8_t dev_addr, uint8_t interface, uint16_t id, bool nintendo);
    ~GameCubeAdapterHost();
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t *out_len);
    bool set_config() override;
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes) override;
    void update(bool full_poll, bool send_events) override;
    void disconnect() override;
    bool tick_digital(proto_Output &type) override { return false; }
    uint16_t tick_analog(proto_Output &type) override { return 0; }

private:
    void update_ports();
    void remove_port(uint8_t port);
    void update_rumble();
    bool m_nintendo;
    uint8_t m_ep_in = 0;
    uint8_t m_ep_out = 0;
    uint16_t m_ep_in_size = 0;
    bool m_out_busy = false;
    // the Nintendo adapter has to be told to start before it sends anything
    bool m_init_sent = false;
    // EVORETRO firmware 0x7 and newer, the only kind that takes rumble
    bool m_evoretro_single_port = false;
    GameCubePortState m_ports[GAMECUBE_ADAPTER_PORTS];
    std::shared_ptr<GameCubePortHost> m_port_hosts[GAMECUBE_ADAPTER_PORTS];
    bool m_port_listed[GAMECUBE_ADAPTER_PORTS] = {};
    // what the rumble report last sent said, so it is only sent again when that changes
    uint8_t m_sent_rumble[GAMECUBE_ADAPTER_RUMBLE_REPORT_LEN] = {};
    bool m_rumble_sent = false;
    uint32_t m_rumble_sent_ms = 0;
    CFG_TUSB_MEM_ALIGN uint8_t m_in_buf[64];
    CFG_TUSB_MEM_ALIGN uint8_t m_out_buf[GAMECUBE_ADAPTER_RUMBLE_REPORT_LEN];
};
