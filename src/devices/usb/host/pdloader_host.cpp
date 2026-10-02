#include "devices/usb/host/pdloader_host.h"
#include "host/usbh_pvt.h"
#include <cstring>

std::shared_ptr<UsbHostInterface> PDLoaderHost::open(std::shared_ptr<UsbHostDevice> device,
    tusb_desc_interface_t const *desc, uint16_t max_len, uint16_t *out_len) {
    uint16_t vid = 0, pid = 0;
    if (!tuh_vid_pid_get(device->dev_addr(), &vid, &pid) ||
        vid != PDLOADER_VID || pid != PDLOADER_PID ||
        desc->bInterfaceClass != TUSB_CLASS_VENDOR_SPECIFIC ||
        desc->bInterfaceSubClass != 0 || desc->bInterfaceProtocol != 0 ||
        desc->bAlternateSetting != 0 || desc->bNumEndpoints != 2)
        return nullptr;

    auto host = std::make_shared<PDLoaderHost>(device->dev_addr(), desc->bInterfaceNumber, device->m_id);
    host->m_vid = vid;
    host->m_pid = pid;
    uint16_t consumed = sizeof(tusb_desc_interface_t);
    auto p = tu_desc_next(reinterpret_cast<uint8_t const *>(desc));
    for (uint8_t i = 0; i < desc->bNumEndpoints; ++i) {
        if (consumed + sizeof(tusb_desc_endpoint_t) > max_len ||
            p[0] < sizeof(tusb_desc_endpoint_t) || tu_desc_type(p) != TUSB_DESC_ENDPOINT)
            return nullptr;
        auto ep = reinterpret_cast<tusb_desc_endpoint_t const *>(p);
        if (ep->bmAttributes.xfer != TUSB_XFER_INTERRUPT || ep->wMaxPacketSize != PDLOADER_PACKET_SIZE)
            return nullptr;
        if (tu_edpt_dir(ep->bEndpointAddress) == TUSB_DIR_IN)
            host->m_ep_in = ep->bEndpointAddress;
        else
            host->m_ep_out = ep->bEndpointAddress;
        if (!tuh_edpt_open(device->dev_addr(), ep))
            return nullptr;
        consumed += ep->bLength;
        p = tu_desc_next(p);
    }
    if (!host->m_ep_in || !host->m_ep_out)
        return nullptr;
    host->m_ep_in_size = PDLOADER_PACKET_SIZE;
    device->host_devices_by_endpoint_in[host->m_ep_in & 0x7f] = host;
    device->host_devices_by_endpoint_out[host->m_ep_out & 0x7f] = host;
    usb_host_add_assignable_interface(host);
    *out_len = consumed;
    return host;
}

bool PDLoaderHost::set_config() {
    UsbHostInterface::set_config();
    return usbh_edpt_xfer(m_dev_addr, m_ep_in, m_buffer, m_ep_in_size);
}

bool PDLoaderHost::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t bytes) {
    if (ep_addr != m_ep_in)
        return true;
    if (result == XFER_RESULT_SUCCESS && bytes == sizeof(m_report) &&
        m_buffer[0] == 0x42 && m_buffer[1] == 0x56 && m_buffer[2] == 0x5a)
        memcpy(&m_report, m_buffer, sizeof(m_report));
    return usbh_edpt_xfer(m_dev_addr, m_ep_in, m_buffer, m_ep_in_size);
}

bool PDLoaderHost::tick_digital(proto_Output &output) {
    if (output.which_mapping == proto_Output_divaTouch_tag) {
        uint32_t electrode = output.mapping.divaTouch;
        return electrode >= 1 && electrode <= 32 &&
            (m_report.slider_touches() & (uint32_t(1) << (electrode - 1))) != 0;
    }
    if (output.which_mapping != proto_Output_gamepadButton_tag)
        return false;
    switch (output.mapping.gamepadButton) {
    case Gamepad_Start: return m_report.buttons1 & (1 << 1);
    case Gamepad_X: return m_report.buttons1 & (1 << 2);
    case Gamepad_B: return m_report.buttons1 & (1 << 3);
    case Gamepad_Y: return m_report.buttons1 & (1 << 4);
    case Gamepad_A: return m_report.buttons1 & (1 << 5);
    case Gamepad_LeftThumbClick: return m_report.buttons2 & (1 << 6);
    case Gamepad_LeftShoulder: return m_report.buttons3_slider1 & 1;
    default: return false;
    }
}

uint16_t PDLoaderHost::tick_analog(proto_Output &output) {
    if (output.which_mapping == proto_Output_gamepadAxis_tag &&
        output.mapping.gamepadAxis == Gamepad_LeftTrigger)
        return (m_report.buttons2 & 0x80) ? UINT16_MAX : 0;
    if (output.which_mapping == proto_Output_divaAxis_tag &&
        output.mapping.divaAxis == ProjectDiva_Slider) {
        uint32_t touches = m_report.slider_touches();
        uint32_t count = 0;
        uint32_t sum_centers = 0;
        for (uint32_t position = 0; position < 32; ++position) {
            if (touches & (uint32_t(1) << position)) {
                ++count;
                sum_centers += 2 * position + 1;
            }
        }
        if (count)
            return (sum_centers * uint32_t(UINT16_MAX)) / (64 * count);
    }
    return 0;
}
