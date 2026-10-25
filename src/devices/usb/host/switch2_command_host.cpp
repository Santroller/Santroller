#include "tusb_option.h"
#include "devices/usb/host/switch2_command_host.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "emulation/usb/usb_devices.h"
#include "devices/usb.hpp"

Switch2CommandHost::Switch2CommandHost(uint8_t dev_addr, uint8_t interface, uint16_t id, std::weak_ptr<UsbHostInterface> hid)
    : UsbHostInterface(dev_addr, interface, id), m_hid(hid)
{
    m_subtype = SubType_Unknown;
    m_has_name = true;
}

std::shared_ptr<UsbHostInterface> Switch2CommandHost::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t *out_len)
{
    TU_VERIFY(TUSB_CLASS_VENDOR_SPECIFIC == itf_desc->bInterfaceClass && itf_desc->bAlternateSetting == 0, nullptr);
    uint8_t dev_addr = list->dev_addr();
    uint16_t vid, pid;
    TU_VERIFY(tuh_vid_pid_get(dev_addr, &vid, &pid), nullptr);
    TU_VERIFY(vid == NINTENDO_VID && (pid == SWITCH_2_PRO_PID || pid == SWITCH_2_JOY_L_PID ||
                                      pid == SWITCH_2_JOY_R_PID || pid == SWITCH_2_GC_PID),
              nullptr);
    const uint16_t drv_len = (uint16_t)(sizeof(tusb_desc_interface_t) + itf_desc->bNumEndpoints * sizeof(tusb_desc_endpoint_t));
    TU_VERIFY(max_len >= drv_len, nullptr);

    // the HID interface comes first, so it is already open
    auto intf = std::make_shared<Switch2CommandHost>(dev_addr, itf_desc->bInterfaceNumber, list->m_id, list->host_devices_by_itf[0]);
    uint8_t const *p_desc = (uint8_t const *)itf_desc;
    uint8_t endpoints = itf_desc->bNumEndpoints;
    while (endpoints--)
    {
        p_desc = tu_desc_next(p_desc);
        tusb_desc_endpoint_t const *desc_ep = (tusb_desc_endpoint_t const *)p_desc;
        TU_VERIFY(TUSB_DESC_ENDPOINT == desc_ep->bDescriptorType, nullptr);
        if (desc_ep->bmAttributes.xfer != TUSB_XFER_BULK)
        {
            continue;
        }
        TU_VERIFY(tuh_edpt_open(dev_addr, desc_ep), nullptr);
        if (desc_ep->bEndpointAddress & 0x80)
        {
            intf->m_ep_in = desc_ep->bEndpointAddress;
            list->host_devices_by_endpoint_in[intf->m_ep_in & (~0x80)] = intf;
        }
        else
        {
            intf->m_ep_out = desc_ep->bEndpointAddress;
            list->host_devices_by_endpoint_out[intf->m_ep_out] = intf;
        }
    }
    TU_VERIFY(intf->m_ep_in && intf->m_ep_out, nullptr);
    printf("switch 2 commands %04x:%04x itf=%d\r\n", vid, pid, itf_desc->bInterfaceNumber);
    *out_len = drv_len;
    return intf;
}

bool Switch2CommandHost::set_config()
{
    UsbHostInterface::set_config();
    m_step = 0;
    send_next();
    return true;
}

void Switch2CommandHost::send_next()
{
    if (m_step >= sizeof(SWITCH2_USB_INIT) / sizeof(SWITCH2_USB_INIT[0]))
    {
        return;
    }
    const Switch2Command &command = SWITCH2_USB_INIT[m_step];
    memcpy(m_out_buf, command.data, command.len);
    if (!usbh_edpt_xfer(m_dev_addr, m_ep_out, m_out_buf, command.len))
    {
        printf("switch 2 command %u failed to send\r\n", m_step);
    }
}

bool Switch2CommandHost::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (ep_addr == m_ep_out)
    {
        // then its reply, which ends with a short packet
        if (result != XFER_RESULT_SUCCESS || !usbh_edpt_xfer(m_dev_addr, m_ep_in, m_in_buf, sizeof(m_in_buf)))
        {
            printf("switch 2 command %u failed: %u\r\n", m_step, unsigned(result));
        }
        return true;
    }
    if (ep_addr != m_ep_in)
    {
        return false;
    }
    if (result == XFER_RESULT_SUCCESS && SWITCH2_USB_INIT[m_step].data == SWITCH2_READ_GYRO_CALIBRATION)
    {
        float bias[3];
        auto hid = m_hid.lock();
        if (hid && switch2_parse_gyro_calibration(m_in_buf, xferred_bytes, bias))
        {
            hid->set_switch2_gyro_bias(bias);
        }
    }
    // a failed command is skipped, as SDL does
    m_step++;
    send_next();
    return true;
}
