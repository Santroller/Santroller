#include "devices/bt/host/wii_host.hpp"
#include "devices/bt/bluetooth_stack.hpp"
#include "managers/device_manager.hpp"
#include "btstack.h"
#include "btstack_config.h"
#include "utils.h"

void reload();

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
    BtStackLock lock;
    m_cmd_buf[0] = 0x00;
    uint8_t res = hid_host_send_report(m_cid, WIIPROTO_REQ_SREQ, m_cmd_buf, 1);
    printf("Wiimote send_status_request: cid=0x%04x status=0x%02x\r\n", m_cid, res);
}

void BtWiiHost::send_init_extension()
{
    BtStackLock lock;
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
    BtStackLock lock;
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
    BtStackLock lock;
    static const uint8_t req[6] = {0x04, 0xa4, 0x00, 0xfa, 0x00, 0x06};
    memcpy(m_cmd_buf, req, sizeof(req));
    uint8_t res = hid_host_send_report(m_cid, WIIPROTO_REQ_RMEM, m_cmd_buf, sizeof(req));
    printf("Wiimote send_read_extension_id: status=0x%02x\r\n", res);
}

void BtWiiHost::send_report_mode(uint8_t mode)
{
    BtStackLock lock;
    m_cmd_buf[0] = 0x00;
    m_cmd_buf[1] = mode;
    uint8_t res = hid_host_send_report(m_cid, WIIPROTO_REQ_DRM, m_cmd_buf, 2);
    printf("Wiimote send_report_mode(0x%02x): status=0x%02x\r\n", mode, res);
}

void BtWiiHost::send_player_led(uint8_t led)
{
    BtStackLock lock;
    m_cmd_buf[0] = led;
    uint8_t res = hid_host_send_report(m_cid, WIIPROTO_REQ_LED, m_cmd_buf, 1);
    printf("Wiimote send_player_led(0x%02x): status=0x%02x\r\n", led, res);
}

void BtWiiHost::send_feedback()
{
    BtStackLock lock;
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
}
void BtWiiHost::send_init_packets()
{
    BluetoothHostInterface::send_init_packets();
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

    switch (report_id)
    {
    case WIIPROTO_REQ_STATUS: // 0x20
    {
        if (len < 4) return;
        uint8_t flags = data[3] & 0x0F;
        bool ext_connected = (flags & 0x02) != 0;

        m_led_sent = false;
        if (ext_connected)
        {
            m_has_ext = true;
            m_ext_init_attempts = 1;
            m_ext_retry_pending = false;
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
        // 0x22 acknowledges a specific output report (byte 3) with an error code (byte 4).
        // Only our memory writes drive the extension handshake, and only if they worked;
        // advancing on anything else reads the ID before the extension is set up.
        if (len < 5 || data[3] != WIIPROTO_REQ_WMEM)
            break;
        if (data[4] != 0)
        {
            if (m_fsm_state == WII_FSM_W4_INIT_ACK || m_fsm_state == WII_FSM_W4_ENC_ACK)
            {
                printf("Wiimote extension write failed: 0x%02x\r\n", data[4]);
                retry_extension_init();
            }
            break;
        }
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
            // byte 3 low nibble is the read error, bytes 4-5 the address read from. A
            // failed read has zeroed data, which would otherwise decode as a Nunchuk.
            uint8_t read_error = data[3] & 0x0F;
            uint16_t read_addr = (uint16_t)((data[4] << 8) | data[5]);
            if (read_error != 0 || read_addr != 0x00FA)
            {
                printf("Wiimote extension ID read failed: err=%u addr=0x%04x\r\n", read_error, read_addr);
                retry_extension_init();
                break;
            }
            m_ext_init_attempts = 0;
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

    case 0x31:
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

void BtWiiHost::retry_extension_init()
{
    if (m_ext_init_attempts >= 5)
    {
        printf("Wiimote extension init gave up after %u attempts\r\n", m_ext_init_attempts);
        m_fsm_state = WII_FSM_READY;
        return;
    }
    // give a freshly inserted extension a moment before trying again
    m_fsm_state = WII_FSM_W4_INIT_ACK;
    m_ext_retry_pending = true;
    m_ext_retry_at = millis() + 100;
}

void BtWiiHost::update(bool full_poll, bool send_events)
{
    BluetoothHostInterface::update(full_poll, send_events);
    if (m_ext_retry_pending && (int32_t)(millis() - m_ext_retry_at) >= 0)
    {
        m_ext_retry_pending = false;
        m_ext_init_attempts++;
        send_init_extension();
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
        if (type.which_mapping == proto_Output_gamepadAxis_tag)
        {
            switch (type.mapping.gamepadAxis)
            {
            case Gamepad_LeftTrigger: return !(d[9] & 0x80);
            case Gamepad_RightTrigger: return !(d[9] & 0x04);
            default: break;
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
                return val << 4;
            }
            case Gamepad_RightStickX:
            {
                uint16_t val = d[2] | ((d[3] & 0x0F) << 8);
                return val << 4;
            }
            case Gamepad_RightStickY:
            {
                uint16_t val = d[6] | ((d[7] & 0x0F) << 8);
                return val << 4;
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

uint16_t BtWiiHost::tick_button_pressure(proto_Output &type)
{
    if (!m_is_pro && (m_decoder.mType == WiiClassicController || m_decoder.mType == WiiClassicControllerPro) &&
        type.which_mapping == proto_Output_gamepadButton_tag)
    {
        switch (type.mapping.gamepadButton)
        {
        case Gamepad_LeftShoulder: return m_decoder.read_button_pressure(WiiButtonClassicLt);
        case Gamepad_RightShoulder: return m_decoder.read_button_pressure(WiiButtonClassicRt);
        default: break;
        }
    }
    return BluetoothHostInterface::tick_button_pressure(type);
}

bool BtWiiHost::tick_axis_digital(proto_Output &type)
{
    if ((m_is_pro || m_decoder.mType == WiiClassicController || m_decoder.mType == WiiClassicControllerPro) &&
        type.which_mapping == proto_Output_gamepadAxis_tag &&
        (type.mapping.gamepadAxis == Gamepad_LeftTrigger || type.mapping.gamepadAxis == Gamepad_RightTrigger))
    {
        return tick_digital(type);
    }
    return BluetoothHostInterface::tick_axis_digital(type);
}
