#pragma once
#include "devices/usb/host/hid/hid_host.h"
#include "protocols/steam_triton.hpp"
#include "emulation/usb/usb_devices.h"

// 2026 Steam Controller (Triton), wired or through a Proteus / Nereid wireless puck
class SteamTritonHost : public HidHost
{
public:
    ~SteamTritonHost() {}
    SteamTritonHost(uint8_t dev_addr, uint8_t interface, uint16_t id, uint16_t vid, uint16_t pid)
        : HidHost(dev_addr, interface, id), m_vid(vid), m_pid(pid)
    {
        m_subtype = SubType_Gamepad;
        // Puck slots only become assignable once a controller links to them
        m_delayed_init = is_dongle(pid);
    }

    static bool is_dongle(uint16_t pid)
    {
        return pid == VALVE_STEAM_PROTEUS_DONGLE_PID || pid == VALVE_STEAM_NEREID_DONGLE_PID;
    }

    bool set_config() override;
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes) override;
    void update(bool full_poll, bool send_events) override;
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list,
                                                   tusb_desc_interface_t const *itf_desc,
                                                   uint16_t max_len,
                                                   uint16_t vid,
                                                   uint16_t pid,
                                                   uint16_t revision,
                                                   HID_ReportInfo_t *info);
    bool tick_digital(proto_Output &type) override;
    uint16_t tick_analog(proto_Output &type) override;

private:
    void set_connected(bool connected);
    void disable_lizard_mode();

    uint16_t m_vid = 0;
    uint16_t m_pid = 0;
    uint8_t m_ep_in = 0;
    uint8_t m_ep_in_size = 0;
    uint32_t m_last_lizard_update = 0;
    CFG_TUSB_MEM_ALIGN uint8_t m_ep_in_buf[64] = {};
    CFG_TUSB_MEM_ALIGN uint8_t m_feature_buf[TRITON_FEATURE_REPORT_LEN] = {};
    SteamTritonState m_state = {};
    bool m_connected = false;
    // set from the transfer callback, the lizard mode command is sent from update()
    volatile bool m_lizard_dirty = false;
};
