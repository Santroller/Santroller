#pragma once
#include "devices/bt/bt_host.hpp"
#include "wii_extension_decoder.hpp"

// ---------------------------------------------------------------------------
// Nintendo Wii Remote & Wii U Pro Controller over BT Classic
// Supports Wiimote core buttons + extensions (Nunchuk, Classic, Guitar, Drums,
// Turntable, Taiko, uDraw, Drawsome) shared with I2C via WiiExtensionDecoder.
// ---------------------------------------------------------------------------
class BtWiiHost : public BluetoothHostInterface
{
public:
    BtWiiHost(uint16_t id, bool is_pro_controller = false);
    ~BtWiiHost() {}

    BtControllerType controller_type() const override
    {
        return m_is_pro ? BtControllerType_BtControllerTypeWiiUPro : BtControllerType_BtControllerTypeWii;
    }

    void on_connected() override;
    void send_init_packets() override;
    void handle_report(const uint8_t *data, uint16_t len) override;

    bool tick_digital(proto_Output &type) override;
    uint16_t tick_analog(proto_Output &type) override;
    bool tick_axis_digital(proto_Output &type) override;
    uint16_t tick_button_pressure(proto_Output &type) override;

    bool is_wii_extension(WiiExtType type) override
    {
        return m_decoder.mType == type;
    }

    void set_rumble(uint8_t left, uint8_t right) override;
    void set_player_led(uint8_t player) override;
    bool has_rumble() const override { return true; }
    bool has_player_led() const override { return true; }
    void update(bool full_poll, bool send_events) override;

    WiiExtensionDecoder m_decoder;

private:
    void send_status_request();
    void send_init_extension();
    void send_disable_encryption();
    void send_read_extension_id();
    void send_report_mode(uint8_t mode);
    void send_player_led(uint8_t led);
    void send_feedback();
    void retry_extension_init();

    bool m_is_pro;
    // extension handshake retries, for an extension that isn't ready yet
    uint8_t m_ext_init_attempts = 0;
    bool m_ext_retry_pending = false;
    uint32_t m_ext_retry_at = 0;
    uint8_t m_player = 0;
    bool m_rumble = false;
    uint8_t m_wii_buttons[2] = {};
    uint8_t m_fsm_state = 0;
    bool m_has_ext = false;
    bool m_led_sent = false;
    uint8_t m_cmd_buf[24] = {};
};
