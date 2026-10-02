#include "devices/usb/host/hid/spice2x_host.h"
#include "host/usbh_pvt.h"
#include "managers/device_manager.hpp"

std::shared_ptr<UsbHostInterface> Spice2xHost::open(std::shared_ptr<UsbHostDevice> device,
    tusb_desc_interface_t const *desc, uint16_t max_len, uint16_t vid, uint16_t pid,
    uint16_t revision, HID_ReportInfo_t *info)
{
    if (vid != SPICE2X_VID || pid != SPICE2X_PID || desc->bInterfaceClass != TUSB_CLASS_HID ||
        desc->bInterfaceSubClass != HID_SUBCLASS_NONE || desc->bInterfaceProtocol != HID_ITF_PROTOCOL_NONE ||
        max_len < sizeof(tusb_desc_interface_t) + sizeof(tusb_hid_descriptor_hid_t))
        return nullptr;
    auto host = std::make_shared<Spice2xHost>(device->dev_addr(), desc->bInterfaceNumber, device->m_id);
    host->m_vid = vid;
    host->m_pid = pid;
    auto p = tu_desc_next(reinterpret_cast<uint8_t const *>(desc));
    if (tu_desc_type(p) != HID_DESC_TYPE_HID ||
        p[0] < sizeof(tusb_hid_descriptor_hid_t) ||
        sizeof(tusb_desc_interface_t) + p[0] > max_len) return nullptr;
    uint16_t consumed = sizeof(tusb_desc_interface_t) + p[0];
    p = tu_desc_next(p);
    for (uint8_t i = 0; i < desc->bNumEndpoints; ++i) {
        if (consumed + sizeof(tusb_desc_endpoint_t) > max_len || tu_desc_type(p) != TUSB_DESC_ENDPOINT)
            return nullptr;
        auto ep = reinterpret_cast<tusb_desc_endpoint_t const *>(p);
        if (ep->bEndpointAddress & 0x80) {
            if (ep->wMaxPacketSize > sizeof(host->m_buffer) || ep->wMaxPacketSize < sizeof(Spice2xInputReport))
                return nullptr;
            host->m_ep_in = ep->bEndpointAddress;
            host->m_ep_in_size = ep->wMaxPacketSize;
            TU_VERIFY(tuh_edpt_open(device->dev_addr(), ep), nullptr);
        }
        consumed += ep->bLength;
        p = tu_desc_next(p);
    }
    if (!host->m_ep_in) return nullptr;
    device->host_devices_by_endpoint_in[host->m_ep_in & 0x7f] = host;
    usb_host_add_assignable_interface(host);
    USB_FreeReportInfo(info);
    return host;
}

bool Spice2xHost::set_config() {
    UsbHostInterface::set_config();
    return usbh_edpt_xfer(m_dev_addr, m_ep_in, m_buffer, m_ep_in_size);
}

bool Spice2xHost::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t bytes) {
    if (ep_addr == m_ep_in) {
        if (result == XFER_RESULT_SUCCESS && bytes >= sizeof(m_report) && m_buffer[0] == SPICE2X_INPUT_ID)
            memcpy(&m_report, m_buffer, sizeof(m_report));
        return usbh_edpt_xfer(m_dev_addr, m_ep_in, m_buffer, m_ep_in_size);
    }
    return true;
}

bool Spice2xHost::tick_digital(proto_Output &output) {
    if (output.which_mapping != proto_Output_gamepadButton_tag) return false;
    auto pad = m_report.pad;
    auto buttons = m_report.controller;
    uint8_t hat = buttons & 0xf;
    switch (output.mapping.gamepadButton) {
    case Gamepad_DpadUp: return (pad & SpiceUp) || hat == 0 || hat == 1 || hat == 7;
    case Gamepad_DpadDown: return (pad & SpiceDown) || hat == 3 || hat == 4 || hat == 5;
    case Gamepad_DpadLeft: return (pad & SpiceLeft) || hat == 5 || hat == 6 || hat == 7;
    case Gamepad_DpadRight: return (pad & SpiceRight) || hat == 1 || hat == 2 || hat == 3;
    case Gamepad_A: return (pad & SpiceDownLeft) || (buttons & (1 << 4));
    case Gamepad_B: return (pad & SpiceDownRight) || (buttons & (1 << 5));
    case Gamepad_X: return (pad & SpiceUpLeft) || (buttons & (1 << 6));
    case Gamepad_Y: return (pad & SpiceUpRight) || (buttons & (1 << 7));
    case Gamepad_LeftThumbClick: return pad & SpiceCenter;
    case Gamepad_Start: return (pad & 1) || (buttons & (1 << 10));
    case Gamepad_Back: return (pad & 2) || (buttons & (1 << 11));
    case Gamepad_LeftShoulder: return buttons & (1 << 9);
    case Gamepad_RightShoulder: return buttons & (1 << 8);
    case Gamepad_Guide: return buttons & (1 << 12);
    case Gamepad_Capture: return buttons & (1 << 13);
    default: return false;
    }
}
