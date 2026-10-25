#include "tusb_option.h"
#include "devices/usb/host/hid/switch_host.h"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "emulation/usb/usb_devices.h"
#include "config/config.hpp"
#include "managers/device_manager.hpp"
#include "hidparser.h"
#include "utils.h"
#include "protocols/switch.hpp"

std::shared_ptr<UsbHostInterface> SwitchHost::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info)
{
    bool is_switch2 = (vid == NINTENDO_VID && (pid == SWITCH_2_PRO_PID || pid == SWITCH_2_JOY_L_PID ||
                                              pid == SWITCH_2_JOY_R_PID || pid == SWITCH_2_GC_PID));
    bool is_switch = false;
    if (vid == NINTENDO_VID)
    {
        is_switch = (pid == SWITCH_PRO_PID || pid == SWITCH_JOYCON_L_PID || pid == SWITCH_JOYCON_R_PID ||
                     pid == SWITCH_CHARGING_GRIP_PID || pid == SWITCH_ONLINE_NES_PID ||
                     pid == SWITCH_ONLINE_SNES_PID || pid == SWITCH_ONLINE_N64_PID ||
                     pid == SWITCH_ONLINE_SEGA_PID || (pid >= 0x2000 && pid <= 0x20FF));
    }
    else if (pid == SWITCH_PRO_PID)
    {
        is_switch = true;
    }

    if (!is_switch)
    {
        return nullptr;
    }

    uint8_t dev_addr = list->dev_addr();

    if (itf_desc->bInterfaceProtocol != HID_ITF_PROTOCOL_NONE)
    {
        return nullptr;
    }

    uint8_t const *p_desc = (uint8_t const *)itf_desc;
    auto intf = std::make_shared<SwitchHost>(dev_addr, itf_desc->bInterfaceNumber, list->m_id, is_switch2);
    uint8_t endpoints = itf_desc->bNumEndpoints;
    p_desc = tu_desc_next(p_desc);
    tusb_hid_descriptor_hid_t *x_desc =
        (tusb_hid_descriptor_hid_t *)p_desc;
    TU_VERIFY(HID_DESC_TYPE_HID == x_desc->bDescriptorType, nullptr);
    while (endpoints--)
    {
        p_desc = tu_desc_next(p_desc);
        tusb_desc_endpoint_t const *desc_ep =
            (tusb_desc_endpoint_t const *)p_desc;
        TU_VERIFY(TUSB_DESC_ENDPOINT == desc_ep->bDescriptorType, nullptr);
        if (desc_ep->bEndpointAddress & 0x80)
        {
            intf->m_ep_in = desc_ep->bEndpointAddress;
            intf->m_ep_in_size = desc_ep->wMaxPacketSize;
            TU_VERIFY(tuh_edpt_open(dev_addr, desc_ep), nullptr);
        }
        else
        {
            intf->m_ep_out = desc_ep->bEndpointAddress;
            intf->m_ep_out_size = desc_ep->wMaxPacketSize;
            TU_VERIFY(tuh_edpt_open(dev_addr, desc_ep), nullptr);
        }
    }
    if (intf->m_ep_out)
    {
        list->host_devices_by_endpoint_out[intf->m_ep_out] = intf;
    }
    if (intf->m_ep_in)
    {
        list->host_devices_by_endpoint_in[intf->m_ep_in & (~0x80)] = intf;
    }
    usb_host_add_assignable_interface(intf);
    USB_FreeReportInfo(info);
    return intf;
}

void SwitchHost::send_handshake_step()
{
    if (!m_ep_out) return;

    switch (m_handshake_step)
    {
    case 0:
    {
        // Handshake 80 02
        static const uint8_t cmd_80_02[] = {0x80, 0x02};
        memcpy(m_ep_out_buf, cmd_80_02, sizeof(cmd_80_02));
        if (usbh_edpt_claim(m_dev_addr, m_ep_out))
        {
            if (usbh_edpt_xfer(m_dev_addr, m_ep_out, m_ep_out_buf, sizeof(cmd_80_02)))
            {
                m_handshake_step++;
            }
            else
            {
                usbh_edpt_release(m_dev_addr, m_ep_out);
            }
        }
        break;
    }
    case 1:
    {
        // Disable USB timeout 80 04
        static const uint8_t cmd_80_04[] = {0x80, 0x04};
        memcpy(m_ep_out_buf, cmd_80_04, sizeof(cmd_80_04));
        if (usbh_edpt_claim(m_dev_addr, m_ep_out))
        {
            if (usbh_edpt_xfer(m_dev_addr, m_ep_out, m_ep_out_buf, sizeof(cmd_80_04)))
            {
                m_handshake_step++;
            }
            else
            {
                usbh_edpt_release(m_dev_addr, m_ep_out);
            }
        }
        break;
    }
    default:
    {
        // Then subcommands: full 0x30 reports, turn the IMU on and read its calibration
        if (m_handshake_step >= SWITCH_HANDSHAKE_STEPS)
        {
            break;
        }
        // a Switch 2 controller's IMU is turned on through its command interface instead, see Switch2CommandHost
        if (m_is_switch2 && m_handshake_step > 2)
        {
            m_handshake_step = SWITCH_HANDSHAKE_STEPS;
            break;
        }
        static const struct
        {
            const uint8_t *data;
            uint8_t len;
        } subcommands[] = {
            {SWITCH_SET_FULL_REPORT_MODE, sizeof(SWITCH_SET_FULL_REPORT_MODE)},
            {SWITCH_ENABLE_IMU, sizeof(SWITCH_ENABLE_IMU)},
            {SWITCH_READ_IMU_FACTORY_CALIBRATION, sizeof(SWITCH_READ_IMU_FACTORY_CALIBRATION)},
            {SWITCH_READ_IMU_USER_CALIBRATION, sizeof(SWITCH_READ_IMU_USER_CALIBRATION)},
        };
        static_assert(sizeof(subcommands) / sizeof(subcommands[0]) == SWITCH_HANDSHAKE_STEPS - 2);
        const auto &subcommand = subcommands[m_handshake_step - 2];
        memcpy(m_ep_out_buf, subcommand.data, subcommand.len);
        if (usbh_edpt_claim(m_dev_addr, m_ep_out))
        {
            if (usbh_edpt_xfer(m_dev_addr, m_ep_out, m_ep_out_buf, subcommand.len))
            {
                m_handshake_step++;
            }
            else
            {
                usbh_edpt_release(m_dev_addr, m_ep_out);
            }
        }
        break;
    }
    }
}

bool SwitchHost::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (!(ep_addr & 0x80))
    {
        m_out_result = result;
        m_out_done.store(true, std::memory_order_release);
    }
    if (ep_addr & 0x80)
    {
        if (m_is_switch2 && result == XFER_RESULT_SUCCESS && xferred_bytes > 1)
        {
            // the same report as over BLE after its report ID
            switch2_parse_report(m_ep_in_buf + 1, xferred_bytes - 1, m_switch2_state);
        }
        else if (result == XFER_RESULT_SUCCESS && xferred_bytes > 0)
        {
            m_imu.parse(m_ep_in_buf, xferred_bytes);
        }
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    else
    {
        if (m_handshake_step < SWITCH_HANDSHAKE_STEPS)
        {
            send_handshake_step();
        }
    }
    return true;
}

void SwitchHost::update(bool full_poll, bool send_events)
{
    UsbHostInterface::update(full_poll, send_events);
    if (m_out_done.exchange(false, std::memory_order_acquire))
    {
        if (m_out_result != XFER_RESULT_SUCCESS)
        {
            m_output.failed(m_pending_output);
            if (!m_out_submit_failed)
                printf("Switch output failed: dev=%u ep=%u result=%u\r\n", m_dev_addr, m_ep_out, unsigned(m_out_result));
            m_out_submit_failed = true;
        }
        m_pending_output = SwitchOutputKind::None;
    }
    if (!m_ep_out || m_handshake_step < SWITCH_HANDSHAKE_STEPS || m_pending_output != SwitchOutputKind::None ||
        usbh_edpt_busy(m_dev_addr, m_ep_out))
        return;

    uint32_t now = millis();
    SwitchOutputKind kind;
    uint8_t len = m_output.build(now, m_ep_out_buf, kind);
    if (!len)
        return;
    m_pending_output = kind;
    if (!send_intr_xfer(m_ep_out, m_ep_out_buf, len))
    {
        m_pending_output = SwitchOutputKind::None;
        if (!m_out_submit_failed)
            printf("Switch output submission failed: dev=%u ep=%u\r\n", m_dev_addr, m_ep_out);
        m_out_submit_failed = true;
        return;
    }
    m_out_submit_failed = false;
    m_output.sent(kind, now);
}

bool SwitchHost::set_config()
{
    UsbHostInterface::set_config();
    if (m_ep_in)
    {
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    m_handshake_step = 0;
    send_handshake_step();
    return true;
}

bool switch_tick_digital(const uint8_t *buf, proto_Output &type)
{
    if (buf[0] == SWITCH_PRO_CON_FULL_REPORT_ID || buf[0] == 0x21)
    {
        auto *data = (const SwitchProGamepad_Data_t *)buf;
        auto &in = data->inputs;

        if (type.which_mapping == proto_Output_gamepadButton_tag)
        {
            switch (type.mapping.gamepadButton)
            {
            case Gamepad_A:               return in.a;
            case Gamepad_B:               return in.b;
            case Gamepad_X:               return in.x;
            case Gamepad_Y:               return in.y;
            case Gamepad_LeftShoulder:    return in.leftShoulder || in.buttonLeftSL;
            case Gamepad_RightShoulder:   return in.rightShoulder || in.buttonRightSL;
            case Gamepad_Back:            return in.back;
            case Gamepad_Start:           return in.start;
            case Gamepad_LeftThumbClick:  return in.leftThumbClick;
            case Gamepad_RightThumbClick: return in.rightThumbClick;
            case Gamepad_Guide:           return in.guide;
            case Gamepad_Capture:         return in.capture;
            case Gamepad_DpadUp:          return in.dpadUp;
            case Gamepad_DpadDown:        return in.dpadDown;
            case Gamepad_DpadLeft:        return in.dpadLeft;
            case Gamepad_DpadRight:       return in.dpadRight;
            default:                      return false;
            }
        }
        return false;
    }

    // Simple HID mode, which the Pro Controller uses over Bluetooth until it is switched to 0x30:
    //   [1] B, A, Y, X, L, R, ZL, ZR  [2] -, +, LS, RS, Home, Capture  [3] hat (0 is up, 8 is neutral)
    //   [4..11] LX, LY, RX, RY (u16, down is the maximum)
    if (buf[0] == SWITCH_PRO_CON_SIMPLE_REPORT_ID)
    {
        uint8_t b1 = buf[1];
        uint8_t b2 = buf[2];
        uint8_t hat = buf[3] & 0x0F;
        bool up = (hat == 0 || hat == 1 || hat == 7);
        bool right = (hat == 1 || hat == 2 || hat == 3);
        bool down = (hat == 3 || hat == 4 || hat == 5);
        bool left = (hat == 5 || hat == 6 || hat == 7);

        if (type.which_mapping == proto_Output_gamepadButton_tag)
        {
            switch (type.mapping.gamepadButton)
            {
            case Gamepad_A:               return (b1 & 0x02) != 0;
            case Gamepad_B:               return (b1 & 0x01) != 0;
            case Gamepad_X:               return (b1 & 0x08) != 0;
            case Gamepad_Y:               return (b1 & 0x04) != 0;
            case Gamepad_LeftShoulder:    return (b1 & 0x10) != 0;
            case Gamepad_RightShoulder:   return (b1 & 0x20) != 0;
            case Gamepad_Back:            return (b2 & 0x01) != 0;
            case Gamepad_Start:           return (b2 & 0x02) != 0;
            case Gamepad_LeftThumbClick:  return (b2 & 0x04) != 0;
            case Gamepad_RightThumbClick: return (b2 & 0x08) != 0;
            case Gamepad_Guide:           return (b2 & 0x10) != 0;
            case Gamepad_Capture:         return (b2 & 0x20) != 0;
            case Gamepad_DpadUp:          return up;
            case Gamepad_DpadDown:        return down;
            case Gamepad_DpadLeft:        return left;
            case Gamepad_DpadRight:       return right;
            default:                      return false;
            }
        }
        return false;
    }

    return false;
}

uint16_t switch_tick_analog(const uint8_t *buf, proto_Output &type, const SwitchImuState *imu)
{
    if (type.which_mapping == proto_Output_gamepadAxis_tag && is_motion_axis(type.mapping.gamepadAxis))
    {
        if (!imu || !imu->has_sample)
        {
            return MOTION_ZERO;
        }
        float accel[3], gyro[3];
        switch_imu_decode(imu->sample, imu->calibration, accel, gyro);
        return motion_host_axis(type.mapping.gamepadAxis, accel, gyro);
    }
    if (buf[0] == SWITCH_PRO_CON_FULL_REPORT_ID || buf[0] == 0x21)
    {
        auto *data = (const SwitchProGamepad_Data_t *)buf;
        auto &in = data->inputs;

        if (type.which_mapping == proto_Output_gamepadAxis_tag)
        {
            switch (type.mapping.gamepadAxis)
            {
            case Gamepad_LeftStickX:   return (uint16_t)(in.leftStickX  << 4);
            case Gamepad_LeftStickY:   return (uint16_t)(in.leftStickY  << 4);
            case Gamepad_RightStickX:  return (uint16_t)(in.rightStickX << 4);
            case Gamepad_RightStickY:  return (uint16_t)(in.rightStickY << 4);
            case Gamepad_LeftTrigger:  return (in.leftTrigger || in.buttonLeftSR) ? 0xFFFF : 0;
            case Gamepad_RightTrigger: return (in.rightTrigger || in.buttonRightSR) ? 0xFFFF : 0;
            default:                   return 0;
            }
        }
        return 0;
    }

    if (buf[0] == SWITCH_PRO_CON_SIMPLE_REPORT_ID)
    {
        if (type.which_mapping == proto_Output_gamepadAxis_tag)
        {
            switch (type.mapping.gamepadAxis)
            {
            case Gamepad_LeftStickX:   return (uint16_t)(buf[4] | (buf[5] << 8));
            case Gamepad_LeftStickY:   return UINT16_MAX - (uint16_t)(buf[6] | (buf[7] << 8));
            case Gamepad_RightStickX:  return (uint16_t)(buf[8] | (buf[9] << 8));
            case Gamepad_RightStickY:  return UINT16_MAX - (uint16_t)(buf[10] | (buf[11] << 8));
            case Gamepad_LeftTrigger:  return (buf[1] & 0x40) ? 0xFFFF : 0;
            case Gamepad_RightTrigger: return (buf[1] & 0x80) ? 0xFFFF : 0;
            default:                   return 0;
            }
        }
        return 0;
    }

    return 0;
}

bool SwitchHost::tick_digital(proto_Output& type)
{
    if (m_is_switch2)
    {
        if (m_ep_in_buf[0] == SWITCH_PRO_CON_FULL_REPORT_ID || m_ep_in_buf[0] == 0x21 || m_ep_in_buf[0] == 0x3F)
        {
            return switch_tick_digital(m_ep_in_buf, type);
        }
        return switch2_tick_digital(m_switch2_state, type);
    }
    return switch_tick_digital(m_ep_in_buf, type);
}

uint16_t SwitchHost::tick_analog(proto_Output& type)
{
    if (m_is_switch2)
    {
        if (m_ep_in_buf[0] == SWITCH_PRO_CON_FULL_REPORT_ID || m_ep_in_buf[0] == 0x21 || m_ep_in_buf[0] == 0x3F)
        {
            return switch_tick_analog(m_ep_in_buf, type, nullptr);
        }
        return switch2_tick_analog(m_switch2_state, type);
    }
    return switch_tick_analog(m_ep_in_buf, type, &m_imu);
}

void SwitchHost::set_rumble(uint8_t left, uint8_t right)
{
    m_output.set_rumble(left, right);
}

void SwitchHost::set_player_led(uint8_t player)
{
    m_output.set_player(player);
}

// Amplitude thresholds from SDL / dekuNukem's rumble_data_table.md, divided by 257 to match our 8 bit
// amplitudes (amp * 257 > t is the same as amp > t / 257 rounded down). Entry i encodes as a high band
// amplitude of 2 * i and a low band amplitude of 0x40 + i / 2, with the low band's ninth bit set for odd i.
static const uint8_t switch_rumble_amp_thresholds[101] = {
    0, 2, 3, 3, 4, 5, 6, 7, 8, 10, 12, 14, 17, 20, 24, 28, 29,
    31, 32, 34, 35, 37, 38, 40, 42, 44, 46, 48, 50, 52, 54, 57, 58, 59,
    61, 62, 63, 65, 66, 68, 69, 71, 72, 74, 76, 77, 79, 81, 82, 84, 86,
    88, 90, 92, 94, 96, 98, 100, 102, 105, 107, 109, 112, 114, 117, 119, 122, 125,
    127, 130, 133, 136, 139, 142, 145, 148, 152, 155, 158, 162, 165, 169, 173, 176, 180,
    184, 188, 192, 197, 201, 205, 210, 215, 219, 224, 229, 234, 239, 244, 250, 255};

static uint8_t switch_rumble_amp_index(uint8_t amp)
{
    uint8_t i = 0;
    while (i < 100 && amp > switch_rumble_amp_thresholds[i])
        i++;
    return i;
}

// Like SDL, the strong motor drives the low band (160Hz) and the weak motor the high band (320Hz), and
// both actuators get the same data
static void switch_encode_rumble(uint8_t strong, uint8_t weak, uint8_t *data)
{
    if (!strong && !weak)
    {
        data[0] = 0x00;
        data[1] = 0x01;
        data[2] = 0x40;
        data[3] = 0x40;
        return;
    }
    uint8_t high = switch_rumble_amp_index(weak);
    uint8_t low = switch_rumble_amp_index(strong);
    data[0] = 0x74;
    data[1] = high * 2;
    data[2] = 0x3D | ((low & 1) ? 0x80 : 0x00);
    data[3] = 0x40 + (low >> 1);
}

uint8_t SwitchOutput::build(uint32_t now, uint8_t *out, SwitchOutputKind &kind) const
{
    kind = SwitchOutputKind::None;
    if (now - last_sent_ms < SWITCH_OUTPUT_MIN_INTERVAL_MS)
        return 0;
    bool rumble_due = rumble_dirty || ((strong || weak) && now - last_sent_ms >= SWITCH_RUMBLE_REFRESH_MS);
    if (!player_dirty && !rumble_due)
        return 0;
    out[0] = player_dirty ? SWITCH_PRO_CON_SUBCMD_REPORT_ID : SWITCH_RUMBLE_REPORT_ID;
    out[1] = counter & 0x0F;
    switch_encode_rumble(strong, weak, out + 2);
    memcpy(out + 6, out + 2, 4);
    if (!player_dirty)
    {
        kind = SwitchOutputKind::Rumble;
        return SWITCH_RUMBLE_REPORT_LEN;
    }
    // Subcommand reports carry rumble data too, so this also takes care of any pending rumble
    out[10] = SWITCH_SUBCMD_SET_PLAYER_LIGHTS;
    out[11] = player >= 1 && player <= 4 ? (1 << player) - 1 : 0;
    kind = SwitchOutputKind::PlayerLed;
    return SWITCH_PLAYER_LED_REPORT_LEN;
}
