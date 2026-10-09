// Link-time fakes for what the GIP sources call outside lib/xgip_protocol and lib/gip_common:
// the TinyUSB device / host endpoint calls, the USB host interface bookkeeping and the
// UsbDevice statics. Everything a fake does is recorded in gip_fake (gip_fakes.hpp).
#include "gip_fakes.hpp"
#include "devices/usb.hpp"
#include "emulation/usb/device.hpp"
#include "usb/auth_broker.h"
#include "managers/config_manager.hpp"
#include "pico/time.h"

std::array<std::shared_ptr<UsbHostDevice>, 127> host_devices;

void gip_fake::reset()
{
    device_mounted = true;
    device_suspended = false;
    device_in_busy = false;
    device_in.clear();
    device_out_arms = 0;
    host_out.clear();
    host_out_fail = false;
    host_in_buffer = nullptr;
    host_in_size = 0;
    host_in_arms = 0;
    enumerating.clear();
    assignable.clear();
    delayed_init_calls = 0;
    for (auto &dev : host_devices)
    {
        if (dev)
        {
            dev->disconnect();
            dev.reset();
        }
    }
    auth_broker.unregister_handler(ModeXboxOne);
    auth_broker.unregister_response_handler(ModeXboxOne);
    auth_broker.set_auth_completed(ModeXboxOne, false);
    ConfigManager::instance().set_current_mode(ModeHid);
    ConfigManager::instance().request_mode(ModeHid);
    ConfigManager::instance().clear_reinit();
    gip_fake_time::now_us = 0;
}

// --- USB host interface bookkeeping (src/devices/usb/host/host.cpp) ---

bool UsbHostInterface::send_intr_xfer(uint8_t endpoint, const void *buffer, uint8_t len)
{
    if (gip_fake::host_out_fail)
        return false;
    const uint8_t *bytes = (const uint8_t *)buffer;
    gip_fake::host_out.push_back({endpoint, std::vector<uint8_t>(bytes, bytes + len)});
    return true;
}

void usb_host_add_assignable_interface(std::shared_ptr<UsbHostInterface> device)
{
    gip_fake::assignable.push_back(device);
}

void usb_host_add_enumerating_interface(std::shared_ptr<UsbHostInterface> device)
{
    gip_fake::enumerating.push_back(device.get());
}

void usb_host_remove_enumerating_interface(UsbHostInterface *device)
{
    std::erase(gip_fake::enumerating, device);
}

void process_delayed_init()
{
    gip_fake::delayed_init_calls++;
}

// --- TinyUSB host ---

extern "C" bool tuh_edpt_open(uint8_t daddr, tusb_desc_endpoint_t const *desc_ep)
{
    (void)daddr;
    (void)desc_ep;
    return true;
}

extern "C" bool usbh_edpt_xfer_with_callback(uint8_t dev_addr, uint8_t ep_addr, uint8_t *buffer, uint16_t total_bytes,
                                             tuh_xfer_cb_t complete_cb, uintptr_t user_data)
{
    (void)dev_addr;
    (void)ep_addr;
    (void)complete_cb;
    (void)user_data;
    gip_fake::host_in_buffer = buffer;
    gip_fake::host_in_size = total_bytes;
    gip_fake::host_in_arms++;
    return true;
}

// --- TinyUSB device ---

extern "C" bool tud_mounted(void)
{
    return gip_fake::device_mounted;
}

extern "C" bool tud_suspended(void)
{
    return gip_fake::device_suspended;
}

extern "C" bool usbd_edpt_xfer(uint8_t rhport, uint8_t ep_addr, uint8_t *buffer, uint16_t total_bytes, bool is_isr)
{
    (void)rhport;
    (void)is_isr;
    if (tu_edpt_dir(ep_addr) == TUSB_DIR_IN)
        gip_fake::device_in.push_back({ep_addr, std::vector<uint8_t>(buffer, buffer + total_bytes)});
    else
        gip_fake::device_out_arms++;
    return true;
}

extern "C" bool usbd_edpt_busy(uint8_t rhport, uint8_t ep_addr)
{
    (void)rhport;
    (void)ep_addr;
    return gip_fake::device_in_busy;
}

extern "C" bool usbd_edpt_claim(uint8_t rhport, uint8_t ep_addr)
{
    (void)rhport;
    (void)ep_addr;
    return true;
}

extern "C" bool usbd_edpt_release(uint8_t rhport, uint8_t ep_addr)
{
    (void)rhport;
    (void)ep_addr;
    return true;
}

extern "C" bool usbd_open_edpt_pair(uint8_t rhport, uint8_t const *p_desc, uint8_t ep_count, uint8_t xfer_type,
                                    uint8_t *ep_out, uint8_t *ep_in)
{
    (void)rhport;
    (void)xfer_type;
    for (uint8_t i = 0; i < ep_count; i++)
    {
        auto const *desc_ep = (tusb_desc_endpoint_t const *)p_desc;
        if (tu_edpt_dir(desc_ep->bEndpointAddress) == TUSB_DIR_IN)
            *ep_in = desc_ep->bEndpointAddress;
        else
            *ep_out = desc_ep->bEndpointAddress;
        p_desc = tu_desc_next(p_desc);
    }
    return true;
}

// --- UsbDevice (src/emulation/usb/device.cpp) ---

uint8_t UsbDevice::m_last_epin = 0x81;
uint8_t UsbDevice::m_last_epout = 0x01;
uint8_t UsbDevice::m_last_strid = 0;
volatile uint32_t UsbDevice::suspend_generation = 0;

void UsbDevice::process_suspended(bool full_poll, bool send_events)
{
    (void)full_poll;
    (void)send_events;
}
