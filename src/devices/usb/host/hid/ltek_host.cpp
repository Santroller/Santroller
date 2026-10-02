#include "tusb_option.h"
#include "devices/usb/host/hid/rhythm_game_host.h"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "emulation/usb/usb_devices.h"
#include "config/config.hpp"
#include "managers/device_manager.hpp"
#include "hidparser.h"
#include "protocols/dance_pad.hpp"


std::shared_ptr<UsbHostInterface> LTekHost::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info)
{
    uint8_t dev_addr = list->dev_addr();

    uint8_t const *p_desc = (uint8_t const *)itf_desc;
    bool lufa = vid == LTEK_LUFA_VID && pid == LTEK_LUFA_PID;
    bool normal = vid == LTEK_VID && pid == LTEK_PID;
    if (itf_desc->bInterfaceProtocol != HID_ITF_PROTOCOL_NONE || !(lufa || normal))
    {
        return nullptr;
    }
    auto intf = std::make_shared<LTekHost>(dev_addr, itf_desc->bInterfaceNumber, list->m_id);
    intf->m_vid = vid;
    intf->m_pid = pid;
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
    intf->m_has_report_id = lufa;
    usb_host_add_assignable_interface(intf);
    USB_FreeReportInfo(info);
    return intf;
}

bool LTekHost::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (ep_addr & 0x80)
    {
        if (result == XFER_RESULT_SUCCESS)
        {
            const auto report_offset = m_has_report_id ? 1u : 0u;
            const auto required_size = report_offset + sizeof(LTEK_Report_Data_t);
            if (xferred_bytes >= required_size &&
                (!m_has_report_id || m_ep_in_buf[0] == LTEK_REPORT_ID))
            {
                const auto *report = reinterpret_cast<const LTEK_Report_Data_t *>(m_ep_in_buf + report_offset);
                m_last_input_report = *report;
            }
        }
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}

bool LTekHost::set_config()
{
    UsbHostInterface::set_config();
    if (m_ep_in)
    {
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}
bool LTekHost::tick_digital(proto_Output& type)
{
    if (type.which_mapping == proto_Output_gamepadButton_tag)
    switch (type.mapping.gamepadButton)
    {
    case Gamepad_A:
        return m_last_input_report.dpadCenter;
    case Gamepad_DpadUp:
        return m_last_input_report.dpadUp;
    case Gamepad_DpadDown:
        return m_last_input_report.dpadDown;
    case Gamepad_DpadLeft:
        return m_last_input_report.dpadLeft;
    case Gamepad_DpadRight:
        return m_last_input_report.dpadRight;
    case Gamepad_Back:
        return m_last_input_report.back;
    case Gamepad_Start:
        return m_last_input_report.start;
    default:
        return false;
    }
    return false;
}
uint16_t LTekHost::tick_analog(proto_Output& type)
{
    return 0;
}