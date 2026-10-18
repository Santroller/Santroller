#include "tusb_option.h"
#include "devices/usb/host/gamecube_adapter_host.h"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "emulation/usb/usb_devices.h"
#include "devices/usb.hpp"
#include "managers/device_manager.hpp"
#include "utils.h"

static const char gamecube_port_name[] = "GameCube Controller ";

GameCubePortHost::GameCubePortHost(uint8_t port, uint8_t dev_addr, uint8_t adapter_itf, uint16_t id, uint16_t vid, uint16_t pid)
    : UsbHostInterface(dev_addr, GAMECUBE_ADAPTER_PORT_INTERFACE(adapter_itf, port), id)
{
    // never configured by the host stack, so take what set_config would have read from the adapter
    m_vid = vid;
    m_pid = pid;
    m_subtype = SubType_Gamepad;
    m_has_name = true;
    set_port_name(port + 1);
}

void GameCubePortHost::set_port_name(uint8_t number)
{
    // a UTF-16LE string descriptor, like a product string read from a device
    memset(m_name, 0, sizeof(m_name));
    const size_t len = sizeof(gamecube_port_name);
    m_name[0] = (len + 1) * 2;
    m_name[1] = TUSB_DESC_STRING;
    for (size_t i = 0; i < len - 1; i++)
    {
        m_name[(i + 1) * 2] = gamecube_port_name[i];
    }
    m_name[len * 2] = '0' + number;
}

bool GameCubePortHost::tick_digital(proto_Output &type)
{
    return gamecube_tick_digital(state, type);
}

uint16_t GameCubePortHost::tick_analog(proto_Output &type)
{
    return gamecube_tick_analog(state, range, type);
}

GameCubeAdapterHost::GameCubeAdapterHost(uint8_t dev_addr, uint8_t interface, uint16_t id, bool nintendo)
    : UsbHostInterface(dev_addr, interface, id), m_nintendo(nintendo)
{
    // The adapter itself isn't a controller; the config tool shows Unknown without a type badge
    m_subtype = SubType_Unknown;
}

GameCubeAdapterHost::~GameCubeAdapterHost()
{
    for (uint8_t i = 0; i < GAMECUBE_ADAPTER_PORTS; i++)
    {
        if (m_port_listed[i])
        {
            usb_host_remove_assignable_interface(m_port_hosts[i].get());
            DeviceManager::instance().remove_device(m_port_hosts[i].get());
        }
    }
    usb_host_remove_enumerating_interface(this);
}

std::shared_ptr<UsbHostInterface> GameCubeAdapterHost::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t *out_len)
{
    TU_VERIFY(TUSB_CLASS_HID == itf_desc->bInterfaceClass && itf_desc->bAlternateSetting == 0, nullptr);
    uint8_t dev_addr = list->dev_addr();
    uint16_t vid, pid;
    TU_VERIFY(tuh_vid_pid_get(dev_addr, &vid, &pid), nullptr);
    bool nintendo = vid == NINTENDO_VID && pid == NINTENDO_GAMECUBE_ADAPTER_PID;
    bool evoretro = vid == DRAGONRISE_VID && (pid == EVORETRO_GAMECUBE_ADAPTER_PID_1 || pid == EVORETRO_GAMECUBE_ADAPTER_PID_2 ||
                                              pid == EVORETRO_GAMECUBE_ADAPTER_PID_3);
    if (!nintendo && !evoretro)
    {
        return nullptr;
    }
    const uint16_t drv_len = (uint16_t)(sizeof(tusb_desc_interface_t) + sizeof(tusb_hid_descriptor_hid_t) +
                                        itf_desc->bNumEndpoints * sizeof(tusb_desc_endpoint_t));
    TU_VERIFY(max_len >= drv_len, nullptr);

    auto intf = std::make_shared<GameCubeAdapterHost>(dev_addr, itf_desc->bInterfaceNumber, list->m_id, nintendo);
    uint8_t const *p_desc = tu_desc_next(itf_desc);
    TU_VERIFY(HID_DESC_TYPE_HID == tu_desc_type(p_desc), nullptr);
    uint8_t endpoints = itf_desc->bNumEndpoints;
    while (endpoints--)
    {
        p_desc = tu_desc_next(p_desc);
        tusb_desc_endpoint_t const *desc_ep = (tusb_desc_endpoint_t const *)p_desc;
        TU_VERIFY(TUSB_DESC_ENDPOINT == desc_ep->bDescriptorType, nullptr);
        TU_VERIFY(tuh_edpt_open(dev_addr, desc_ep), nullptr);
        if (desc_ep->bEndpointAddress & 0x80)
        {
            intf->m_ep_in = desc_ep->bEndpointAddress;
            intf->m_ep_in_size = tu_min16(tu_edpt_packet_size(desc_ep), sizeof(intf->m_in_buf));
            list->host_devices_by_endpoint_in[intf->m_ep_in & (~0x80)] = intf;
        }
        else
        {
            intf->m_ep_out = desc_ep->bEndpointAddress;
            list->host_devices_by_endpoint_out[intf->m_ep_out] = intf;
        }
    }
    for (uint8_t i = 0; i < GAMECUBE_ADAPTER_PORTS; i++)
    {
        intf->m_port_hosts[i] = std::make_shared<GameCubePortHost>(i, dev_addr, itf_desc->bInterfaceNumber, list->m_id, vid, pid);
    }
    printf("gamecube adapter %04x:%04x itf=%d\r\n", vid, pid, itf_desc->bInterfaceNumber);
    // its ports become assignable as controllers are plugged in
    usb_host_add_enumerating_interface(intf);
    *out_len = drv_len;
    return intf;
}

bool GameCubeAdapterHost::set_config()
{
    if (m_nintendo)
    {
        // SET_PROTOCOL to report protocol, as SDL does before starting it. Sent before the name is read, which
        // holds the control endpoint until it is done
        tusb_control_request_t set_protocol = {
            bmRequestType_bit : {
                recipient : TUSB_REQ_RCPT_INTERFACE,
                type : TUSB_REQ_TYPE_CLASS,
                direction : TUSB_DIR_OUT
            },
            bRequest : HID_REQ_CONTROL_SET_PROTOCOL,
            wValue : HID_PROTOCOL_REPORT,
            wIndex : m_interface,
            wLength : 0
        };
        send_ctrl_xfer(set_protocol, nullptr, nullptr);
    }
    UsbHostInterface::set_config();
    if (m_ep_in)
    {
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_in_buf, m_ep_in_size);
    }
    // the init for the Nintendo adapter goes out from update
    return true;
}

bool GameCubeAdapterHost::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (!(ep_addr & 0x80))
    {
        m_out_busy = false;
        return true;
    }
    if (result == XFER_RESULT_SUCCESS)
    {
        if (m_nintendo)
        {
            gamecube_adapter_parse(m_in_buf, xferred_bytes, m_ports);
        }
        else
        {
            uint8_t port;
            if (evoretro_parse(m_in_buf, xferred_bytes, m_ports, port))
            {
                m_evoretro_single_port = xferred_bytes == EVORETRO_REPORT_LEN;
            }
        }
        for (uint8_t i = 0; i < GAMECUBE_ADAPTER_PORTS; i++)
        {
            m_port_hosts[i]->state = m_ports[i];
            if (m_port_listed[i])
            {
                m_port_hosts[i]->range.update(m_ports[i]);
            }
        }
    }
    usbh_edpt_xfer(m_dev_addr, m_ep_in, m_in_buf, m_ep_in_size);
    return true;
}

void GameCubeAdapterHost::update(bool full_poll, bool send_events)
{
    UsbHostInterface::update(full_poll, send_events);
    if (m_nintendo && !m_init_sent && m_ep_out && !m_out_busy)
    {
        m_out_buf[0] = GAMECUBE_ADAPTER_INIT;
        if (send_intr_xfer(m_ep_out, m_out_buf, 1))
        {
            m_out_busy = true;
            m_init_sent = true;
        }
    }
    update_ports();
    update_rumble();
}

void GameCubeAdapterHost::update_ports()
{
    for (uint8_t i = 0; i < GAMECUBE_ADAPTER_PORTS; i++)
    {
        auto &port = m_port_hosts[i];
        if (m_ports[i].connected && !m_port_listed[i])
        {
            m_port_listed[i] = true;
            port->range.reset();
            port->range.update(m_ports[i]);
            // a port per interface, so number it after the interface instead
            port->set_port_name(m_evoretro_single_port ? m_interface + 1 : i + 1);
            usb_host_add_assignable_interface(port);
            process_delayed_init();
            port->send_hotplug_event(true);
        }
        else if (!m_ports[i].connected && m_port_listed[i])
        {
            remove_port(i);
            process_delayed_init();
        }
    }
}

void GameCubeAdapterHost::remove_port(uint8_t i)
{
    auto &port = m_port_hosts[i];
    m_port_listed[i] = false;
    usb_host_remove_assignable_interface(port.get());
    DeviceManager::instance().remove_device(port.get());
    port->set_rumble(0, 0);
    // a profile may still hold the port, so leave it reading as a controller with nothing pressed
    port->state = GameCubePortState();
    port->disconnect();
}

void GameCubeAdapterHost::disconnect()
{
    for (uint8_t i = 0; i < GAMECUBE_ADAPTER_PORTS; i++)
    {
        if (m_port_listed[i])
        {
            remove_port(i);
        }
    }
    UsbHostInterface::disconnect();
}

void GameCubeAdapterHost::update_rumble()
{
    if (!m_ep_out || m_out_busy || usbh_edpt_busy(m_dev_addr, m_ep_out))
    {
        return;
    }
    uint8_t report[GAMECUBE_ADAPTER_RUMBLE_REPORT_LEN];
    uint8_t len;
    uint32_t now = millis();
    if (m_nintendo)
    {
        if (!m_init_sent)
        {
            return;
        }
        bool rumble[GAMECUBE_ADAPTER_PORTS];
        for (uint8_t i = 0; i < GAMECUBE_ADAPTER_PORTS; i++)
        {
            rumble[i] = m_port_listed[i] && m_port_hosts[i]->rumble();
        }
        gamecube_adapter_rumble_report(rumble, m_ports, report);
        len = GAMECUBE_ADAPTER_RUMBLE_REPORT_LEN;
        if (m_rumble_sent && !memcmp(report, m_sent_rumble, len))
        {
            return;
        }
    }
    else
    {
        if (!m_evoretro_single_port)
        {
            return;
        }
        bool rumble = m_port_listed[0] && m_port_hosts[0]->rumble();
        evoretro_rumble_report(rumble, report);
        len = EVORETRO_RUMBLE_REPORT_LEN;
        bool changed = !m_rumble_sent || memcmp(report, m_sent_rumble, len);
        if (!changed && !(rumble && now - m_rumble_sent_ms >= EVORETRO_RUMBLE_REFRESH_MS))
        {
            return;
        }
    }
    memcpy(m_out_buf, report, len);
    if (!send_intr_xfer(m_ep_out, m_out_buf, len))
    {
        return;
    }
    m_out_busy = true;
    memcpy(m_sent_rumble, report, len);
    m_rumble_sent = true;
    m_rumble_sent_ms = now;
}
