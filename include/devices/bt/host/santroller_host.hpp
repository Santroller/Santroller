#pragma once
#include "devices/bt/bt_host.hpp"
#include "protocols/santroller_output.hpp"

#define ARDWIINO_VID     0x1209
#define ARDWIINO_PID     0x2882
#define ARDWIINO_PID_BLE 0x2885

// ---------------------------------------------------------------------------
// Santroller 1 & 2 BLE Host
// ---------------------------------------------------------------------------
class BleSantrollerHost : public BluetoothHostInterface
{
public:
    BleSantrollerHost(uint16_t id, bool is_v2, uint16_t version, SubType known_subtype = SubType_Unknown);
    ~BleSantrollerHost() override = default;

    BtControllerType controller_type() const override { return BtControllerType_BtControllerTypeSantroller; }

    void on_connected() override;
    void handle_report(const uint8_t *data, uint16_t len) override;

    bool tick_digital(proto_Output &type) override;
    uint16_t tick_analog(proto_Output &type) override;
    uint16_t tick_button_pressure(proto_Output &type) override;

    void update(bool full_poll, bool send_events) override;
    void set_rumble(uint8_t left, uint8_t right) override { sync_output(); m_output.set_rumble(left, right); }
    void set_player_led(uint8_t player) override { m_output.set_player(player); }
    void set_lightbar(uint8_t r, uint8_t g, uint8_t b) override { m_output.set_rgb(r, g, b); }
    void set_euphoria_led(bool state) override { sync_output(); m_output.set_euphoria(state); }
    void set_stagekit_led(uint8_t param, uint8_t command) override { sync_output(); m_output.set_stagekit(param, command); }
    bool has_rumble() const override { return m_output.has_rumble(); }
    bool has_player_led() const override { return m_output.supports(CapabilityHasStandardPlayerLeds); }
    bool has_lightbar() const override { return m_output.supports(CapabilityHasRGBIndicatorLed); }
    bool has_euphoria_led() const override { return m_output.has_euphoria(); }
    bool has_stagekit_led() const override { return m_output.has_stagekit(); }

private:
    void sync_output() { m_output.subtype = m_subtype; }
    SantrollerOutputState m_output;
    void handle_report_v1(const uint8_t *data, uint16_t len);
    void handle_report_v2(const uint8_t *data, uint16_t len);

    bool m_is_v2 = false;
    uint8_t m_capabilities = 0;
    uint8_t m_query_attempts = 0;
};
