#include "tusb_option.h"
#include "devices/usb/host/hid/switch_host.h"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "emulation/usb/usb_devices.h"
#include "config/config.hpp"
#include "managers/device_manager.hpp"
#include "hidparser.h"
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
    case 2:
    {
        // Set standard report mode 0x30 via output report 0x01
        static const uint8_t cmd_mode_30[] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x30};
        memcpy(m_ep_out_buf, cmd_mode_30, sizeof(cmd_mode_30));
        if (usbh_edpt_claim(m_dev_addr, m_ep_out))
        {
            if (usbh_edpt_xfer(m_dev_addr, m_ep_out, m_ep_out_buf, sizeof(cmd_mode_30)))
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
        break;
    }
}

bool SwitchHost::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (ep_addr & 0x80)
    {
        if (m_is_switch2 && result == XFER_RESULT_SUCCESS && xferred_bytes > 0)
        {
            switch2_parse_report(m_ep_in_buf, xferred_bytes, m_switch2_state);
        }
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    else
    {
        if (m_handshake_step < 3)
        {
            send_handshake_step();
        }
    }
    return true;
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

    if (buf[0] == 0x3F)
    {
        uint8_t b1 = buf[1];
        uint8_t b2 = buf[2];
        uint8_t b3 = buf[3];
        uint8_t hat = b3 & 0x0F;
        bool up = (hat == 0 || hat == 1 || hat == 7);
        bool right = (hat == 1 || hat == 2 || hat == 3);
        bool down = (hat == 3 || hat == 4 || hat == 5);
        bool left = (hat == 5 || hat == 6 || hat == 7);

        if (type.which_mapping == proto_Output_gamepadButton_tag)
        {
            switch (type.mapping.gamepadButton)
            {
            case Gamepad_A:               return (b1 & 0x08) != 0;
            case Gamepad_B:               return (b1 & 0x04) != 0;
            case Gamepad_X:               return (b1 & 0x02) != 0;
            case Gamepad_Y:               return (b1 & 0x01) != 0;
            case Gamepad_LeftShoulder:    return (b3 & 0x40) != 0;
            case Gamepad_RightShoulder:   return (b1 & 0x40) != 0;
            case Gamepad_Back:            return (b2 & 0x01) != 0;
            case Gamepad_Start:           return (b2 & 0x02) != 0;
            case Gamepad_LeftThumbClick:  return (b2 & 0x08) != 0;
            case Gamepad_RightThumbClick: return (b2 & 0x04) != 0;
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

uint16_t switch_tick_analog(const uint8_t *buf, proto_Output &type)
{
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

    if (buf[0] == 0x3F)
    {
        if (type.which_mapping == proto_Output_gamepadAxis_tag)
        {
            switch (type.mapping.gamepadAxis)
            {
            case Gamepad_LeftStickX:   return (uint16_t)buf[4] * 0x101;
            case Gamepad_LeftStickY:   return (uint16_t)(UINT8_MAX - buf[5]) * 0x101;
            case Gamepad_RightStickX:  return (uint16_t)buf[6] * 0x101;
            case Gamepad_RightStickY:  return (uint16_t)(UINT8_MAX - buf[7]) * 0x101;
            case Gamepad_LeftTrigger:  return (buf[3] & 0x80) ? 0xFFFF : 0;
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
            return switch_tick_analog(m_ep_in_buf, type);
        }
        return switch2_tick_analog(m_switch2_state, type);
    }
    return switch_tick_analog(m_ep_in_buf, type);
}

static inline void encode_switch_rumble_data(uint8_t amp, uint8_t *data)
{
    if (amp == 0)
    {
        data[0] = 0x00;
        data[1] = 0x01;
        data[2] = 0x40;
        data[3] = 0x40;
        return;
    }
    uint8_t hf_amp = amp;
    data[0] = 0x00;
    data[1] = (hf_amp & 0xFE) | 0x01;
    data[2] = 0x40;
    data[3] = 0x40 + (amp >> 1);
}

void SwitchHost::set_rumble(uint8_t left, uint8_t right)
{
    m_rumble_left = left;
    m_rumble_right = right;
    if (!m_ep_out) return;

    uint8_t buf[10] = {};
    buf[0] = 0x10;
    buf[1] = m_packet_counter++ & 0x0F;
    encode_switch_rumble_data(left, buf + 2);
    encode_switch_rumble_data(right, buf + 6);

    if (usbh_edpt_claim(m_dev_addr, m_ep_out))
    {
        memcpy(m_ep_out_buf, buf, sizeof(buf));
        if (!usbh_edpt_xfer(m_dev_addr, m_ep_out, m_ep_out_buf, sizeof(buf)))
        {
            usbh_edpt_release(m_dev_addr, m_ep_out);
            return;
        }
    }
}

void SwitchHost::set_player_led(uint8_t player)
{
    if (!m_ep_out) return;

    uint8_t mask = 0;
    if (player == 1) mask = 0x01;
    else if (player == 2) mask = 0x03;
    else if (player == 3) mask = 0x07;
    else if (player == 4) mask = 0x0F;

    uint8_t buf[12] = {};
    buf[0] = 0x01;
    buf[1] = m_packet_counter++ & 0x0F;
    encode_switch_rumble_data(m_rumble_left, buf + 2);
    encode_switch_rumble_data(m_rumble_right, buf + 6);
    buf[10] = 0x30;
    buf[11] = mask;

    if (usbh_edpt_claim(m_dev_addr, m_ep_out))
    {
        memcpy(m_ep_out_buf, buf, sizeof(buf));
        if (!usbh_edpt_xfer(m_dev_addr, m_ep_out, m_ep_out_buf, sizeof(buf)))
        {
            usbh_edpt_release(m_dev_addr, m_ep_out);
            return;
        }
    }
}