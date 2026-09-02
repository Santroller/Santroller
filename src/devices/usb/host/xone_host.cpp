#include "tusb_option.h"
#include "devices/usb/host/xone_host.h"
#include "usb/auth_broker.h"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "emulation/usb/usb_devices.h"
#include "devices/usb.hpp"
#include "config/config.hpp"
#include "managers/device_manager.hpp"
#include "utils.h"

extern "C" {
#include "gip_packet_handler.h"
#include "gip_device.h"
#include "gip_button_mapping.h"
#include "gip_device_mappings.h"
}

#include <algorithm>

static void xone_on_device_descriptor_wrapper(void *context, SubType subtype);
static void xone_on_arrival_wrapper(void *context);
static void xone_queue_packet_wrapper(void *context, const uint8_t *data, uint16_t len);

static const gip_device_interface_t xone_gip_interface = {
    .on_device_descriptor = xone_on_device_descriptor_wrapper,
    .on_arrival = xone_on_arrival_wrapper,
    .queue_packet = xone_queue_packet_wrapper
};

XboxOneHost::XboxOneHost(uint8_t dev_addr, uint8_t interface, uint16_t id) : UsbHostInterface(dev_addr, interface, id)
{
    m_delayed_init = true;
    gip_device_init(&m_gip_device);
    m_gip_device.user_context = this;
    m_gip_device.interface = &xone_gip_interface;
    m_report_queue = gip_report_queue_create();
}

XboxOneHost::~XboxOneHost()
{
    if (m_auth_registered)
    {
        auth_broker.unregister_handler(ModeXboxOne);
        m_auth_registered = false;
    }

    gip_device_cleanup(&m_gip_device);
    gip_report_queue_destroy(m_report_queue);
}

void XboxOneHost::disconnect()
{
    if (m_auth_registered)
    {
        auth_broker.unregister_handler(ModeXboxOne);
        m_auth_registered = false;
    }

    UsbHostInterface::disconnect();
}

void XboxOneHost::send_report_from_host(XGIPProtocol *report)
{
    m_gip_device.outgoing_xgip->copyAttributes(report);
    gip_report_queue_push(m_report_queue, 
                          m_gip_device.outgoing_xgip->generatePacket(), 
                          m_gip_device.outgoing_xgip->getPacketLength());
}

std::shared_ptr<UsbHostInterface> XboxOneHost::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *desc_itf, uint16_t max_len, uint16_t *out_len)
{
    uint32_t size = desc_itf->bLength;
    TU_VERIFY(desc_itf->bInterfaceSubClass == 0x47 &&
                  desc_itf->bInterfaceProtocol == 0xD0 && desc_itf->bAlternateSetting == 0 && desc_itf->bInterfaceNumber == 0,
              nullptr);
    uint8_t dev_addr = list->dev_addr();

    uint8_t const *p_desc = (uint8_t const *)desc_itf;

    auto intf = std::make_shared<XboxOneHost>(dev_addr, desc_itf->bInterfaceNumber, list->m_id);
    intf->m_subtype = SubType_Gamepad;
    uint8_t endpoints = desc_itf->bNumEndpoints;
    while (endpoints--)
    {
        p_desc = tu_desc_next(p_desc);
        tusb_desc_endpoint_t const *desc_ep =
            (tusb_desc_endpoint_t const *)p_desc;
        size += desc_ep->bLength;
        TU_VERIFY(TUSB_DESC_ENDPOINT == desc_ep->bDescriptorType, nullptr);

        if (desc_ep->bEndpointAddress & 0x80)
        {
            intf->m_ep_in = desc_ep->bEndpointAddress;
            intf->m_ep_in_size = desc_ep->wMaxPacketSize;
            TU_VERIFY(tuh_edpt_open(dev_addr, desc_ep), nullptr);
            usbh_edpt_xfer(dev_addr, intf->m_ep_in, intf->m_ep_in_buf, intf->m_ep_in_size);
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
    if (desc_itf->bInterfaceNumber == 0)
    {
        usb_host_add_enumerating_interface(intf);
        
        // Register as auth provider if not already registered
        if (!auth_broker.has_handler(ModeXboxOne))
        {
            auth_broker.register_handler(ModeXboxOne, [intf](XGIPProtocol* packet) {
                intf->send_report_from_host(packet);
            });
            intf->m_auth_registered = true;
        }
        
        // Auth registration handled by auth_broker above
    }
    printf("size: %d\r\n", size);
    *out_len = size;
    return intf;
}

bool XboxOneHost::set_config()
{
    printf("set config\r\n");
    memset(m_last_inputs, 0, sizeof(m_last_inputs));
    
    UsbHostInterface::set_config();
    return true;
}

// Device descriptor callback
static void xone_on_device_descriptor_wrapper(void *context, SubType subtype)
{
    XboxOneHost *host = (XboxOneHost *)context;
    
    if (subtype != Unknown) {
        host->set_subtype(subtype);
        host->m_gip_device.subtype = subtype;
        
        // Move from enumerating to assignable
        usb_host_remove_enumerating_interface(host);
        usb_host_add_assignable_interface(host_devices[host->dev_addr()]->host_devices_by_itf[host->interface()]);
        
        // Send power-on sequence using device interface
        gip_send_power_on_sequence(&host->m_gip_device);
        process_delayed_init();
    }
}

// Arrival callback
static void xone_on_arrival_wrapper(void *context)
{
    XboxOneHost *host = (XboxOneHost *)context;
    if (host) {
        gip_default_arrival_callback(&host->m_gip_device, xone_queue_packet_wrapper);
    }
}

// Queue packet callback
static void xone_queue_packet_wrapper(void *context, const uint8_t *data, uint16_t len)
{
    XboxOneHost *host = (XboxOneHost *)context;
    if (!host || !gip_report_queue_push(host->m_report_queue, data, len))
    {
        printf("XboxOneHost: Failed to queue GIP packet len=%d\r\n", len);
    }
}

bool XboxOneHost::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (ep_addr & 0x80 && result != XFER_RESULT_FAILED)
    {
        if (xferred_bytes == 0)
        {
            usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
            return true;
        }
        
        gip_device_process_incoming(&m_gip_device, m_ep_in_buf, xferred_bytes);

        uint8_t *ack_data;
        uint16_t ack_len;
        if (gip_device_generate_ack(&m_gip_device, &ack_data, &ack_len))
        {
            gip_report_queue_push(m_report_queue, ack_data, ack_len);
        }

        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}

void XboxOneHost::update(bool full_poll, bool send_events)
{
    UsbHostInterface::update(full_poll, send_events);
    uint32_t now = to_ms_since_boot(get_absolute_time());
    
    // Send queued reports
    if (!gip_report_queue_empty(m_report_queue))
    {
        const gip_report_queue_item_t* item = gip_report_queue_front(m_report_queue);
        if (item && send_intr_xfer(m_ep_out, item->report, item->len))
        {
            gip_report_queue_pop(m_report_queue);
        }
    }
    
    // Update GIP device - handles ACK timeout and outgoing packets
    gip_device_update_with_queue(&m_gip_device, now, XGIP_ACK_WAIT_TIMEOUT, m_report_queue);
}

bool XboxOneHost::tick_digital(proto_Output &type)
{
    // Use shared GIP button mapping with shared device raw input
    return gip_tick_digital(m_gip_device.raw_input, m_subtype, &type);
}
uint16_t XboxOneHost::tick_analog(proto_Output &type)
{
    // Use shared GIP axis mapping with shared device raw input
    return gip_tick_analog(m_gip_device.raw_input, m_subtype, &type);
}