#include "devices/bt/bt_classic_host.hpp"
#include "devices/bt/bt_host.hpp"
#include "managers/device_manager.hpp"
#include "hidparser.h"
#include "btstack.h"
#include "btstack_config.h"
#include "protocols/ps3.hpp"
#include "protocols/ps4.hpp"
#include "protocols/switch.hpp"
#include "utils.h"
#include "emulation/usb/usb_devices.h"

void reload();

// Include USB host headers for shared tick free functions.
// These headers pull in TinyUSB but they come AFTER btstack.h here.
// Since btstack defines HID_REPORT_TYPE_* as an enum and tinyusb
// defines them as a conflicting enum, we avoid including both.
// The USB host headers only need to be included for the function DECLARATIONS;
// the actual implementations live in the USB host .cpp files.
// Include order matters: btstack.h first, then we need a trick:
// We define TUSB_CONFIG_FILE to suppress tusb_option.h from being loaded.
// Actually the simplest fix: just declare the shared functions ourselves.
bool ps3_tick_digital(const uint8_t *buf, SubType subtype, bool third_party, proto_Output &type, bool wt = false);
uint16_t ps3_tick_analog(const uint8_t *buf, SubType subtype, bool third_party, proto_Output &type);
bool ps4_tick_digital(const uint8_t *buf, SubType subtype, bool third_party, proto_Output &type, uint32_t *last_ghl_poke);
uint16_t ps4_tick_analog(const uint8_t *buf, SubType subtype, bool third_party, proto_Output &type);
bool ps5_tick_digital(const uint8_t *buf, SubType subtype, bool third_party, proto_Output &type);
uint16_t ps5_tick_analog(const uint8_t *buf, SubType subtype, bool third_party, proto_Output &type);
bool switch_tick_digital(const uint8_t *buf, proto_Output &type);
uint16_t switch_tick_analog(const uint8_t *buf, proto_Output &type);
bool ps4_parse_capabilities(const uint8_t *data, uint16_t len, uint16_t vid, uint16_t pid,
                            SubType &subtype, bool &sensors, bool &lightbar, bool &vibration, bool &touchpad);
bool ps5_parse_capabilities(const uint8_t *data, uint16_t len,
                            SubType &subtype, bool &sensors, bool &lightbar, bool &vibration, bool &touchpad);

extern "C" {
#include "gip_device.h"
#include "gip_report_queue.h"
#include "gip_button_mapping.h"
#include "gip_device_mappings.h"
}

#include <memory>

// ============================================================================
// BtDs3Host
// ============================================================================

void BtDs3Host::on_connected()
{
    // Enable DS3 HID reports — the same command sent for USB DS3
    static const uint8_t enable[] = {0x42, 0x0c, 0x00, 0x00};
    hid_host_send_set_report(m_cid, HID_REPORT_TYPE_FEATURE, 0xF4, enable, sizeof(enable));
}

bool BtDs3Host::tick_digital(proto_Output &type)
{
    return ps3_tick_digital(m_report_buf, m_subtype, false, type);
}

uint16_t BtDs3Host::tick_analog(proto_Output &type)
{
    return ps3_tick_analog(m_report_buf, m_subtype, false, type);
}

// ============================================================================
// BtDs4Host
// ============================================================================

// BT Classic DS4 uses report 0x11:
//   byte 0: 0xa1 (HID input report prefix)
//   byte 1: 0x11 (report id)
//   bytes 2–3: unknown
//   bytes 4+: same as USB 0x01 report (leftStickX, leftStickY, …)
//   last 4 bytes: CRC32
// We normalise it so m_report_buf looks like a USB 0x01 report
// (report_id=0x01, leftStickX, …) so we can reuse PS4 tick logic unchanged.
void BtDs4Host::handle_report(const uint8_t *data, uint16_t len)
{
    if (len < 2) return;

    // Detect BT-specific framing: first byte is 0x11 when BTstack strips the
    // leading 0xa1, otherwise it arrives as plain report already.
    if (data[0] == 0x11 && len >= 12)
    {
        // Strip 3-byte BT header, copy as report_id=0x01 + payload, drop 4-byte CRC tail
        uint16_t raw_payload_len = (len >= 7) ? (len - 7) : 0;
        uint16_t payload_len = raw_payload_len < (sizeof(m_report_buf) - 1)
                                   ? raw_payload_len
                                   : (sizeof(m_report_buf) - 1);
        m_report_buf[0] = 0x01;
        memcpy(m_report_buf + 1, data + 3, payload_len);
    }
    else
    {
        // Already in USB format (or unknown; just copy verbatim)
        BluetoothHostInterface::handle_report(data, len);
    }
}

void BtDs4Host::on_connected()
{
    BluetoothHostInterface::on_connected();
}

void BtDs4Host::request_capabilities()
{
}

void BtDs4Host::handle_feature_report(const uint8_t *data, uint16_t len)
{
    if (m_third_party)
    {
        printf("PS4 Feature Report received (len=%d): ", len);
        for (int i = 0; i < len; i++)
            printf("%02x ", data[i]);
        printf("\r\n");

        SubType sub = m_subtype;
        bool sens = m_sensors_supported;
        bool light = m_lightbar_supported;
        bool vib = m_vibration_supported;
        bool touch = m_touchpad_supported;
        if (ps4_parse_capabilities(data, len, m_vid, m_pid, sub, sens, light, vib, touch))
        {
            m_subtype = sub;
            m_sensors_supported = sens;
            m_lightbar_supported = light;
            m_vibration_supported = vib;
            m_touchpad_supported = touch;
            printf("PS4 3rd-party capabilities: subtype=%d, sensors=%d, light=%d, vib=%d, touch=%d\r\n",
                   (int)m_subtype, (int)m_sensors_supported, (int)m_lightbar_supported,
                   (int)m_vibration_supported, (int)m_touchpad_supported);
        }
        else
        {
            printf("PS4 3rd-party capabilities: parse failed, keeping subtype=%d\r\n", (int)m_subtype);
        }
        m_ready = true;
    }
}

void BtDs4Host::handle_feature_report_failed()
{
    m_ready = true;
}

bool BtDs4Host::tick_digital(proto_Output &type)
{
    return ps4_tick_digital(m_report_buf, m_subtype, m_third_party, type, nullptr);
}

uint16_t BtDs4Host::tick_analog(proto_Output &type)
{
    return ps4_tick_analog(m_report_buf, m_subtype, m_third_party, type);
}

// ============================================================================
// BtDs5Host (DualSense over BT Classic)
// ============================================================================

void BtDs5Host::handle_report(const uint8_t *data, uint16_t len)
{
    if (len < 2) return;

    if (data[0] == 0x31 && len >= 11)
    {
        uint16_t raw_payload_len = (len >= 2) ? (len - 2) : 0;
        uint16_t payload_len = raw_payload_len < (sizeof(m_report_buf) - 1)
                                   ? raw_payload_len
                                   : (sizeof(m_report_buf) - 1);
        m_report_buf[0] = 0x01;
        memcpy(m_report_buf + 1, data + 2, payload_len);
    }
    else
    {
        BluetoothHostInterface::handle_report(data, len);
    }
}

void BtDs5Host::on_connected()
{
    BluetoothHostInterface::on_connected();
}

void BtDs5Host::request_capabilities()
{
}

void BtDs5Host::handle_feature_report(const uint8_t *data, uint16_t len)
{
    if (m_third_party)
    {
        SubType sub = m_subtype;
        bool sens = m_sensors_supported;
        bool light = m_lightbar_supported;
        bool vib = m_vibration_supported;
        bool touch = m_touchpad_supported;
        if (ps5_parse_capabilities(data, len, sub, sens, light, vib, touch))
        {
            m_subtype = sub;
            m_sensors_supported = sens;
            m_lightbar_supported = light;
            m_vibration_supported = vib;
            m_touchpad_supported = touch;
            printf("PS5 3rd-party capabilities: subtype=%d, sensors=%d, light=%d, vib=%d, touch=%d\r\n",
                   (int)m_subtype, (int)m_sensors_supported, (int)m_lightbar_supported,
                   (int)m_vibration_supported, (int)m_touchpad_supported);
        }
        else
        {
            printf("PS5 3rd-party capabilities: parse failed, defaulting to subtype=%d\r\n", (int)m_subtype);
        }
        m_ready = true;
    }
}

void BtDs5Host::handle_feature_report_failed()
{
    m_ready = true;
}

bool BtDs5Host::tick_digital(proto_Output &type)
{
    return ps5_tick_digital(m_report_buf, m_subtype, m_third_party, type);
}

uint16_t BtDs5Host::tick_analog(proto_Output &type)
{
    return ps5_tick_analog(m_report_buf, m_subtype, m_third_party, type);
}

// ============================================================================
// BtSwitchHost
// ============================================================================

void BtSwitchHost::on_connected()
{
    // Switch Pro: send USB mode command so it sends full 0x30 reports
    // CMD 0x03 sets the input report mode; 0x30 = full controller state
    static const uint8_t cmd[] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x30};
    hid_host_send_set_report(m_cid, HID_REPORT_TYPE_OUTPUT, cmd[0], cmd + 1, sizeof(cmd) - 1);
}

void BtSwitchHost::handle_report(const uint8_t *data, uint16_t len)
{
    BluetoothHostInterface::handle_report(data, len);
    if (m_is_switch2 && len > 0)
    {
        switch2_parse_report(data, len, m_switch2_state);
    }
}

bool BtSwitchHost::tick_digital(proto_Output &type)
{
    if (m_is_switch2)
    {
        if (m_report_buf[0] == SWITCH_PRO_CON_FULL_REPORT_ID || m_report_buf[0] == 0x21 || m_report_buf[0] == 0x3F)
        {
            return switch_tick_digital(m_report_buf, type);
        }
        return switch2_tick_digital(m_switch2_state, type);
    }
    return switch_tick_digital(m_report_buf, type);
}

uint16_t BtSwitchHost::tick_analog(proto_Output &type)
{
    if (m_is_switch2)
    {
        if (m_report_buf[0] == SWITCH_PRO_CON_FULL_REPORT_ID || m_report_buf[0] == 0x21 || m_report_buf[0] == 0x3F)
        {
            return switch_tick_analog(m_report_buf, type);
        }
        return switch2_tick_analog(m_switch2_state, type);
    }
    return switch_tick_analog(m_report_buf, type);
}

// ============================================================================
// BtXboxOneHost (GIP over BT Classic)
// ============================================================================

static void btxone_on_device_descriptor(void *ctx, SubType subtype);
static void btxone_on_arrival(void *ctx);
static void btxone_queue_packet(void *ctx, const uint8_t *data, uint16_t len);
static void btxone_send_ack(void *ctx, const uint8_t *data, uint16_t len);

static const gip_device_interface_t btxone_gip_interface = {
    .on_device_descriptor = btxone_on_device_descriptor,
    .on_arrival           = btxone_on_arrival,
    .queue_packet         = btxone_queue_packet,
    .send_ack             = btxone_send_ack,
};

BtXboxOneHost::BtXboxOneHost(uint16_t id) : BluetoothHostInterface(id)
{
    m_subtype = SubType_Gamepad;
    gip_device_init(&m_gip_device);
    m_gip_device.user_context = this;
    m_gip_device.interface    = &btxone_gip_interface;
    m_report_queue            = gip_report_queue_create();
}

BtXboxOneHost::~BtXboxOneHost()
{
    gip_device_cleanup(&m_gip_device);
    gip_report_queue_destroy(m_report_queue);
}

void BtXboxOneHost::on_connected()
{
    gip_default_arrival_callback(&m_gip_device, btxone_queue_packet);
}

void BtXboxOneHost::on_disconnected()
{
    gip_device_cleanup(&m_gip_device);
    BluetoothHostInterface::on_disconnected();
}

void BtXboxOneHost::handle_report(const uint8_t *data, uint16_t len)
{
    gip_device_process_incoming(&m_gip_device, data, len);
}

void BtXboxOneHost::update(bool full_poll, bool send_events)
{
    BluetoothHostInterface::update(full_poll, send_events);
    uint32_t now = to_ms_since_boot(get_absolute_time());

    // Drain the outgoing GIP packet queue via HID set-report
    if (!gip_report_queue_empty(m_report_queue))
    {
        const gip_report_queue_item_t *item = gip_report_queue_front(m_report_queue);
        if (item)
        {
            send_hid_output(item->report, item->len);
            gip_report_queue_pop(m_report_queue);
        }
    }
    gip_device_update_with_queue(&m_gip_device, now, XGIP_ACK_WAIT_TIMEOUT, m_report_queue);
}

void BtXboxOneHost::send_hid_output(const uint8_t *data, uint16_t len)
{
    if (m_cid && len > 0 && len <= sizeof(m_out_buf))
    {
        memcpy(m_out_buf, data, len);
        hid_host_send_set_report(m_cid, HID_REPORT_TYPE_OUTPUT, m_out_buf[0],
                                 m_out_buf + 1, len - 1);
    }
}

bool BtXboxOneHost::tick_digital(proto_Output &type)
{
    return gip_tick_digital(m_gip_device.raw_input, m_subtype, &type);
}

uint16_t BtXboxOneHost::tick_analog(proto_Output &type)
{
    return gip_tick_analog(m_gip_device.raw_input, m_subtype, &type);
}

static void btxone_on_device_descriptor(void *ctx, SubType subtype)
{
    auto *host = (BtXboxOneHost *)ctx;
    if (subtype != Unknown)
    {
        host->m_subtype             = subtype;
        host->m_gip_device.subtype  = subtype;
        gip_send_power_on_sequence(&host->m_gip_device);
    }
}

static void btxone_on_arrival(void *ctx)
{
    auto *host = (BtXboxOneHost *)ctx;
    gip_default_arrival_callback(&host->m_gip_device, btxone_queue_packet);
}

static void btxone_queue_packet(void *ctx, const uint8_t *data, uint16_t len)
{
    auto *host = (BtXboxOneHost *)ctx;
    gip_report_queue_push(host->m_report_queue, data, len);
}

static void btxone_send_ack(void *ctx, const uint8_t *data, uint16_t len)
{
    auto *host = (BtXboxOneHost *)ctx;
    gip_report_queue_push_front(host->m_report_queue, data, len);
}

// ============================================================================
// BtGenericHost
// ============================================================================

BtGenericHost::~BtGenericHost()
{
    if (m_info)
    {
        USB_FreeReportInfo(m_info);
        m_info = nullptr;
    }
}

void BtGenericHost::handle_report(const uint8_t *data, uint16_t len)
{
    BluetoothHostInterface::handle_report(data, len);
    m_data = {};
    fill_generic_report(m_info, m_report_buf, &m_data);
}

bool BtGenericHost::tick_digital(proto_Output &type)
{
    if (type.which_mapping == proto_Output_gamepadButton_tag)
    {
        switch (type.mapping.gamepadButton)
        {
        case Gamepad_A:               return (m_data.genericButtons & (1 << 0)) != 0;
        case Gamepad_B:               return (m_data.genericButtons & (1 << 1)) != 0;
        case Gamepad_X:               return (m_data.genericButtons & (1 << 2)) != 0;
        case Gamepad_Y:               return (m_data.genericButtons & (1 << 3)) != 0;
        case Gamepad_LeftShoulder:    return (m_data.genericButtons & (1 << 4)) != 0;
        case Gamepad_RightShoulder:   return (m_data.genericButtons & (1 << 5)) != 0;
        case Gamepad_Back:            return (m_data.genericButtons & (1 << 6)) != 0;
        case Gamepad_Start:           return (m_data.genericButtons & (1 << 7)) != 0;
        case Gamepad_LeftThumbClick:  return (m_data.genericButtons & (1 << 8)) != 0;
        case Gamepad_RightThumbClick: return (m_data.genericButtons & (1 << 9)) != 0;
        case Gamepad_Guide:           return (m_data.genericButtons & (1 << 10)) != 0;
        case Gamepad_Capture:         return (m_data.genericButtons & (1 << 11)) != 0;
        case Gamepad_DpadUp:          return m_data.dpadUp != 0;
        case Gamepad_DpadDown:        return m_data.dpadDown != 0;
        case Gamepad_DpadLeft:        return m_data.dpadLeft != 0;
        case Gamepad_DpadRight:       return m_data.dpadRight != 0;
        default:                      return false;
        }
    }
    return false;
}

uint16_t BtGenericHost::tick_analog(proto_Output &type)
{
    if (type.which_mapping == proto_Output_gamepadAxis_tag)
    {
        switch (type.mapping.gamepadAxis)
        {
        case Gamepad_LeftStickX:   return m_data.genericAxisX;
        case Gamepad_LeftStickY:   return m_data.genericAxisY;
        case Gamepad_RightStickX:  return m_data.genericAxisRx;
        case Gamepad_RightStickY:  return m_data.genericAxisRy;
        case Gamepad_LeftTrigger:  return m_data.genericAxisZ;
        case Gamepad_RightTrigger: return m_data.genericAxisRz;
        default:                   return 0;
        }
    }
    return 0;
}

// ============================================================================
// BtWiiHost (Nintendo Wii Remote & Wii U Pro Controller)
// ============================================================================

#define WIIPROTO_REQ_LED     0x11
#define WIIPROTO_REQ_DRM     0x12
#define WIIPROTO_REQ_SREQ    0x15
#define WIIPROTO_REQ_WMEM    0x16
#define WIIPROTO_REQ_RMEM    0x17
#define WIIPROTO_REQ_STATUS  0x20
#define WIIPROTO_REQ_DATA    0x21
#define WIIPROTO_REQ_RETURN  0x22

enum {
    WII_FSM_IDLE = 0,
    WII_FSM_W4_STATUS,
    WII_FSM_W4_INIT_ACK,
    WII_FSM_W4_ENC_ACK,
    WII_FSM_W4_EXT_ID,
    WII_FSM_READY
};

BtWiiHost::BtWiiHost(uint16_t id, bool is_pro_controller)
    : BluetoothHostInterface(id), m_is_pro(is_pro_controller)
{
    m_subtype = SubType_Gamepad;
    if (!m_is_pro)
    {
        m_ready = false;
    }
}

void BtWiiHost::send_status_request()
{
    m_cmd_buf[0] = 0x00;
    uint8_t res = hid_host_send_report(m_cid, WIIPROTO_REQ_SREQ, m_cmd_buf, 1);
    printf("Wiimote send_status_request: cid=0x%04x status=0x%02x\r\n", m_cid, res);
}

void BtWiiHost::send_init_extension()
{
    memset(m_cmd_buf, 0, 21);
    m_cmd_buf[0] = 0x04;
    m_cmd_buf[1] = 0xa4;
    m_cmd_buf[2] = 0x00;
    m_cmd_buf[3] = 0xf0;
    m_cmd_buf[4] = 0x01;
    m_cmd_buf[5] = 0x55;
    uint8_t res = hid_host_send_report(m_cid, WIIPROTO_REQ_WMEM, m_cmd_buf, 21);
    printf("Wiimote send_init_extension: status=0x%02x\r\n", res);
}

void BtWiiHost::send_disable_encryption()
{
    memset(m_cmd_buf, 0, 21);
    m_cmd_buf[0] = 0x04;
    m_cmd_buf[1] = 0xa4;
    m_cmd_buf[2] = 0x00;
    m_cmd_buf[3] = 0xfb;
    m_cmd_buf[4] = 0x01;
    m_cmd_buf[5] = 0x00;
    uint8_t res = hid_host_send_report(m_cid, WIIPROTO_REQ_WMEM, m_cmd_buf, 21);
    printf("Wiimote send_disable_encryption: status=0x%02x\r\n", res);
}

void BtWiiHost::send_read_extension_id()
{
    static const uint8_t req[6] = {0x04, 0xa4, 0x00, 0xfa, 0x00, 0x06};
    memcpy(m_cmd_buf, req, sizeof(req));
    uint8_t res = hid_host_send_report(m_cid, WIIPROTO_REQ_RMEM, m_cmd_buf, sizeof(req));
    printf("Wiimote send_read_extension_id: status=0x%02x\r\n", res);
}

void BtWiiHost::send_report_mode(uint8_t mode)
{
    m_cmd_buf[0] = 0x00;
    m_cmd_buf[1] = mode;
    uint8_t res = hid_host_send_report(m_cid, WIIPROTO_REQ_DRM, m_cmd_buf, 2);
    printf("Wiimote send_report_mode(0x%02x): status=0x%02x\r\n", mode, res);
}

void BtWiiHost::send_player_led(uint8_t led)
{
    m_cmd_buf[0] = led;
    uint8_t res = hid_host_send_report(m_cid, WIIPROTO_REQ_LED, m_cmd_buf, 1);
    printf("Wiimote send_player_led(0x%02x): status=0x%02x\r\n", led, res);
}

void BtWiiHost::send_feedback()
{
    uint8_t val = (m_rumble ? 0x01 : 0x00);
    if (m_player == 1) val |= 0x10;
    else if (m_player == 2) val |= 0x20;
    else if (m_player == 3) val |= 0x40;
    else if (m_player == 4) val |= 0x80;
    m_cmd_buf[0] = val;
    hid_host_send_report(m_cid, WIIPROTO_REQ_LED, m_cmd_buf, 1);
}

void BtWiiHost::set_rumble(uint8_t left, uint8_t right)
{
    m_rumble = (left > 0 || right > 0);
    send_feedback();
}

void BtWiiHost::set_player_led(uint8_t player)
{
    m_player = player;
    send_feedback();
}

void BtWiiHost::on_connected()
{
    BluetoothHostInterface::on_connected();
    m_led_sent = false;
    if (m_is_pro)
    {
        send_report_mode(0x34);
        m_subtype = SubType_Gamepad;
        set_ready(true);
        m_fsm_state = WII_FSM_READY;
    }
    else
    {
        m_fsm_state = WII_FSM_W4_STATUS;
        send_status_request();
    }
}

void BtWiiHost::handle_report(const uint8_t *data, uint16_t len)
{
    if (len > 0 && data[0] == 0xa1)
    {
        data++;
        len--;
    }
    if (len < 1) return;

    uint8_t report_id = data[0];
    printf("Wiimote report: id=0x%02x len=%u fsm=%d\r\n", report_id, len, m_fsm_state);

    switch (report_id)
    {
    case WIIPROTO_REQ_STATUS: // 0x20
    {
        if (len < 4) return;
        uint8_t flags = data[3] & 0x0F;
        bool ext_connected = (flags & 0x02) != 0;
        printf("Wiimote status: flags=0x%02x ext=%d\r\n", flags, (int)ext_connected);

        m_led_sent = false;
        if (ext_connected)
        {
            m_has_ext = true;
            m_fsm_state = WII_FSM_W4_INIT_ACK;
            send_init_extension();
        }
        else
        {
            m_has_ext = false;
            m_decoder.reset();
            m_subtype = SubType_Gamepad;
            send_report_mode(0x30);
            if (!m_ready)
            {
                set_ready(true);
                printf("Wiimote ready (standalone), subtype=%d\r\n", (int)m_subtype);
            }
            else
            {
                notify_subtype_changed();
                reload();
            }
            m_fsm_state = WII_FSM_READY;
        }
        break;
    }

    case WIIPROTO_REQ_RETURN: // 0x22
    {
        printf("Wiimote ack (0x22), fsm=%d\r\n", m_fsm_state);
        if (m_fsm_state == WII_FSM_W4_INIT_ACK)
        {
            m_fsm_state = WII_FSM_W4_ENC_ACK;
            send_disable_encryption();
        }
        else if (m_fsm_state == WII_FSM_W4_ENC_ACK)
        {
            m_fsm_state = WII_FSM_W4_EXT_ID;
            send_read_extension_id();
        }
        break;
    }

    case WIIPROTO_REQ_DATA: // 0x21
    {
        if (len >= 12 && m_fsm_state == WII_FSM_W4_EXT_ID)
        {
            m_decoder.decode_id(data + 6);
            m_led_sent = false;
            if (data[10] == 0x01 && data[11] == 0x20)
            {
                m_is_pro = true;
                m_subtype = SubType_Gamepad;
                send_report_mode(0x34);
            }
            else
            {
                m_subtype = m_decoder.get_subtype();
                send_report_mode(0x32);
            }
            if (!m_ready)
            {
                set_ready(true);
                printf("Wiimote ready (extension), subtype=%d\r\n", (int)m_subtype);
            }
            else
            {
                notify_subtype_changed();
                reload();
            }
            m_fsm_state = WII_FSM_READY;
        }
        break;
    }

    case 0x30:
    {
        if (!m_led_sent)
        {
            m_led_sent = true;
            send_player_led(0x10);
        }
        if (len >= 3)
        {
            m_wii_buttons[0] = data[1];
            m_wii_buttons[1] = data[2];
        }
        break;
    }

    case 0x32:
    {
        if (!m_led_sent)
        {
            m_led_sent = true;
            send_player_led(0x10);
        }
        if (len >= 3)
        {
            m_wii_buttons[0] = data[1];
            m_wii_buttons[1] = data[2];
        }
        if (len >= 11)
        {
            m_decoder.update_data(data + 3, 8, this);
        }
        break;
    }

    case 0x34:
    {
        if (!m_led_sent)
        {
            m_led_sent = true;
            send_player_led(0x10);
        }
        BluetoothHostInterface::handle_report(data, len);
        if (len >= 3)
        {
            m_wii_buttons[0] = data[1];
            m_wii_buttons[1] = data[2];
        }
        break;
    }

    default:
        break;
    }
}

bool BtWiiHost::tick_digital(proto_Output &type)
{
    if (m_is_pro && m_report_buf[0] == 0x34)
    {
        const uint8_t *d = m_report_buf + 3;
        if (type.which_mapping == proto_Output_gamepadButton_tag)
        {
            switch (type.mapping.gamepadButton)
            {
            case Gamepad_A:               return !(d[9] & 0x10);
            case Gamepad_B:               return !(d[9] & 0x40);
            case Gamepad_X:               return !(d[9] & 0x08);
            case Gamepad_Y:               return !(d[9] & 0x20);
            case Gamepad_DpadRight:       return !(d[8] & 0x80);
            case Gamepad_DpadDown:        return !(d[8] & 0x40);
            case Gamepad_DpadLeft:        return !(d[9] & 0x02);
            case Gamepad_DpadUp:          return !(d[9] & 0x01);
            case Gamepad_LeftShoulder:    return !(d[8] & 0x20);
            case Gamepad_RightShoulder:   return !(d[8] & 0x02);
            case Gamepad_LeftThumbClick:  return !(d[10] & 0x02);
            case Gamepad_RightThumbClick: return !(d[10] & 0x01);
            case Gamepad_Start:           return !(d[8] & 0x04);
            case Gamepad_Back:            return !(d[8] & 0x10);
            case Gamepad_Guide:           return !(d[8] & 0x08);
            default:                      return false;
            }
        }
        return false;
    }

    if (m_decoder.mType != WiiExtType::WiiNoExtension)
    {
        if (m_decoder.tick_digital(type))
            return true;
    }

    if (type.which_mapping == proto_Output_gamepadButton_tag)
    {
        uint8_t b0 = m_wii_buttons[0];
        uint8_t b1 = m_wii_buttons[1];
        switch (type.mapping.gamepadButton)
        {
        case Gamepad_DpadLeft:      return (b0 & 0x01) != 0;
        case Gamepad_DpadRight:     return (b0 & 0x02) != 0;
        case Gamepad_DpadDown:      return (b0 & 0x04) != 0;
        case Gamepad_DpadUp:        return (b0 & 0x08) != 0;
        case Gamepad_Start:         return (b0 & 0x10) != 0;
        case Gamepad_A:             return (b1 & 0x08) != 0;
        case Gamepad_B:             return (b1 & 0x04) != 0;
        case Gamepad_X:             return (b1 & 0x02) != 0;
        case Gamepad_Y:             return (b1 & 0x01) != 0;
        case Gamepad_Back:          return (b1 & 0x10) != 0;
        case Gamepad_Guide:         return (b1 & 0x80) != 0;
        default:                    return false;
        }
    }

    return false;
}

uint16_t BtWiiHost::tick_analog(proto_Output &type)
{
    if (m_is_pro && m_report_buf[0] == 0x34)
    {
        const uint8_t *d = m_report_buf + 3;
        if (type.which_mapping == proto_Output_gamepadAxis_tag)
        {
            switch (type.mapping.gamepadAxis)
            {
            case Gamepad_LeftStickX:
            {
                uint16_t val = d[0] | ((d[1] & 0x0F) << 8);
                return val << 4;
            }
            case Gamepad_LeftStickY:
            {
                uint16_t val = d[4] | ((d[5] & 0x0F) << 8);
                return 65535 - (val << 4);
            }
            case Gamepad_RightStickX:
            {
                uint16_t val = d[2] | ((d[3] & 0x0F) << 8);
                return val << 4;
            }
            case Gamepad_RightStickY:
            {
                uint16_t val = d[6] | ((d[7] & 0x0F) << 8);
                return 65535 - (val << 4);
            }
            case Gamepad_LeftTrigger:
                return !(d[9] & 0x80) ? 65535 : 0;
            case Gamepad_RightTrigger:
                return !(d[9] & 0x04) ? 65535 : 0;
            default:
                return 0;
            }
        }
        return 0;
    }

    if (m_decoder.mType != WiiExtType::WiiNoExtension)
    {
        return m_decoder.tick_analog(type);
    }

    return 0;
}

// ============================================================================
// Factory
// ============================================================================

std::shared_ptr<BluetoothHostInterface> bt_classic_create_host(uint16_t vid, uint16_t pid,
                                                                uint16_t version,
                                                                uint16_t device_id,
                                                                HID_ReportInfo_t *info,
                                                                SubType known_subtype,
                                                                bool known_ready,
                                                                const char *dev_name,
                                                                BtControllerType known_controller_type)
{
    std::shared_ptr<BluetoothHostInterface> host = nullptr;

    // DS3 / DualShock 3 & Navigation Controller
    if (vid == SONY_VID && (pid == SONY_DS3_PID || pid == SONY_PS3_NAV_PID))
    {
        if (info) USB_FreeReportInfo(info);
        host = std::make_shared<BtDs3Host>(device_id);
    }
    // DS4 instruments
    else if ((vid == MADCATZ_VID && (pid == PS4_STRAT_PID || pid == PS4_MADCATZ_DRUM_PID)) ||
             (vid == PDP_VID && pid == PS4_JAG_PID))
    {
        SubType sub = (vid == MADCATZ_VID && pid == PS4_MADCATZ_DRUM_PID) ? RockBandDrums : RockBandGuitar;
        if (info) USB_FreeReportInfo(info);
        host = std::make_shared<BtDs4Host>(device_id, sub,
                                           true, false, false, false, false,
                                           vid, pid);
    }
    // DS4 / DualShock 4 gamepads (first-party or clones/generic DS4)
    else if ((vid == SONY_VID && (pid == SONY_DS4_PID_1 || pid == SONY_DS4_PID_2 || pid == SONY_DS4_PID_3)) ||
             (info && info->foundPS4Usage) ||
             known_controller_type == BtControllerType_BtControllerTypePS4 ||
             (known_subtype != SubType_Unknown && (vid == SONY_VID || (info && info->foundPS4Usage))))
    {
        SubType sub = (known_subtype != SubType_Unknown) ? known_subtype : SubType_Gamepad;
        if (info) USB_FreeReportInfo(info);
        host = std::make_shared<BtDs4Host>(device_id, sub,
                                           false, true, true, true, true,
                                           vid, pid);
    }
    // DS5 / DualSense gamepads
    else if ((vid == SONY_VID && (pid == SONY_DS5_PID || pid == SONY_DS5_EDGE_PID)) ||
             (info && info->foundPS5Usage) || known_controller_type == BtControllerType_BtControllerTypePS5)
    {
        SubType sub = (known_subtype != SubType_Unknown) ? known_subtype : SubType_Gamepad;
        if (info) USB_FreeReportInfo(info);
        host = std::make_shared<BtDs5Host>(device_id, sub,
                                           false, true, true, true, true,
                                           vid, pid);
    }
    // Switch Pro Controller & Joy-Cons & Switch 2 (matched by VID/PID or name for clones)
    else if ((vid == NINTENDO_VID && (pid == SWITCH_PRO_PID || pid == SWITCH_JOYCON_L_PID ||
                                      pid == SWITCH_JOYCON_R_PID || pid == SWITCH_CHARGING_GRIP_PID ||
                                      pid == SWITCH_ONLINE_NES_PID || pid == SWITCH_ONLINE_SNES_PID ||
                                      pid == SWITCH_ONLINE_N64_PID || pid == SWITCH_ONLINE_SEGA_PID ||
                                      (pid >= 0x2000 && pid <= 0x20FF))) ||
             pid == SWITCH_PRO_PID ||
             is_switch_name(dev_name) ||
             known_controller_type == BtControllerType_BtControllerTypeSwitch)
    {
        if (info) USB_FreeReportInfo(info);
        bool is_switch2 = (vid == NINTENDO_VID && (pid == SWITCH_2_PRO_PID || pid == SWITCH_2_JOY_L_PID ||
                                                  pid == SWITCH_2_JOY_R_PID || pid == SWITCH_2_GC_PID)) ||
                          (dev_name && strstr(dev_name, "Switch 2") != nullptr);
        host = std::make_shared<BtSwitchHost>(device_id, is_switch2);
    }
    // HORI Wireless HORIPAD for Steam or Valve devices
    else if ((vid == HORI_VID && pid == HORI_STEAM_CONTROLLER_PID) ||
             vid == VALVE_USB_VID)
    {
        auto generic_host = std::make_shared<BtGenericHost>(device_id, info);
        generic_host->m_subtype = SubType_Gamepad;
        host = generic_host;
    }
    // Nintendo Wii Remote
    else if (vid == NINTENDO_VID && pid == WII_REMOTE_PID)
    {
        if (info) USB_FreeReportInfo(info);
        host = std::make_shared<BtWiiHost>(device_id, false);
    }
    // Nintendo Wii U Pro Controller
    else if (vid == NINTENDO_VID && pid == WII_U_PRO_PID)
    {
        if (info) USB_FreeReportInfo(info);
        host = std::make_shared<BtWiiHost>(device_id, true);
    }
    // Xbox Wireless Controllers over Bluetooth Classic use standard HID reports
    else if (vid == XBOX_VID)
    {
        auto generic_host = std::make_shared<BtGenericHost>(device_id, info);
        generic_host->m_subtype = SubType_Gamepad;
        host = generic_host;
    }
    else
    {
        // Generic HID fallback
        auto generic_host = std::make_shared<BtGenericHost>(device_id, info);
        if (known_subtype != SubType_Unknown)
        {
            generic_host->m_subtype = known_subtype;
        }
        host = generic_host;
    }

    if (host)
    {
        if (!vid && !pid && host->controller_type() == BtControllerType_BtControllerTypeSwitch)
        {
            host->m_vid = NINTENDO_VID;
            host->m_pid = switch_pid_from_name(dev_name);
        }
        else
        {
            host->m_vid = vid;
            host->m_pid = pid;
        }
    }
    return host;
}
