#include "emulation/usb/midi_device.h"
#include <string.h>
#include "managers/profile_manager.hpp"
#include "device/usbd.h"
#include "device/usbd_pvt.h"
#include "class/audio/audio.h"
#include "class/midi/midi.h"
#include "pico/time.h"

UsbMidiDevice::UsbMidiDevice()
{
}

void UsbMidiDevice::initialize()
{
    m_epin = next_epin();
    m_epout = next_epout();
    m_strid = next_strid();
    ProfileManager::instance().map_usb_instance_epin(m_epin, interface_id);
    ProfileManager::instance().map_usb_instance_epout(m_epout, interface_id);
    m_state.clear_all();
    m_output.reset();
}

void UsbMidiDevice::process(bool full_poll, bool send_events)
{
    if (tud_suspended())
    {
        process_suspended(full_poll, send_events);
        return;
    }
    if (!tud_ready())
    {
        for (const auto &profile : profiles)
        {
            for (const auto &led : profile->leds)
            {
                led->update(full_poll, send_events);
            }
        }
        return;
    }
    m_state.clear_all();
    for (const auto &profile : profiles)
    {
        profile->reset_drum_state();
        profile->midi_state.clear_all();
        for (const auto &mapping : profile->mappings)
        {
            mapping->update(full_poll, send_events);
            mapping->update_hid(m_scratch_report);
        }
        for (const auto &led : profile->leds)
        {
            led->update(full_poll, send_events);
        }
        m_state.merge(profile->midi_state);
    }
    m_output.collect(m_state, time_us_64());
    send_pending();
}

void UsbMidiDevice::send_pending()
{
    MidiMessage message;
    if (!m_output.pending(message) || !usbd_edpt_claim(TUD_OPT_RHPORT, m_epin))
    {
        return;
    }
    // as many messages as fit in one packet
    uint16_t len = 0;
    do
    {
        midi_usb_packet(message, 0, m_epin_buf + len);
        len += 4;
        m_output.sent(message);
    } while (len < sizeof(m_epin_buf) && m_output.pending(message));
    usbd_edpt_xfer(TUD_OPT_RHPORT, m_epin, m_epin_buf, len, false);
}

size_t UsbMidiDevice::compatible_section_descriptor(uint8_t *dest, size_t remaining)
{
    return 0;
}

size_t UsbMidiDevice::config_descriptor(uint8_t *dest, size_t remaining)
{
    uint8_t desc[] = {TUD_MIDI_DESCRIPTOR(interface_id, m_strid, m_epout, m_epin, USB_MIDI_EP_SIZE)};
    assert(sizeof(desc) <= remaining);
    memcpy(dest, desc, sizeof(desc));
    return sizeof(desc);
}

size_t UsbMidiDevice::device_name(uint8_t idx, char *desc)
{
    if (idx == m_strid)
    {
        memcpy(desc, profiles[0]->name, sizeof(profiles[0]->name));
        return sizeof(profiles[0]->name);
    }
    return 0;
}

void UsbMidiDevice::device_descriptor(tusb_desc_device_t *desc)
{
}

// Takes both interfaces, so tinyusb hands this device the requests for either of them
uint16_t UsbMidiDevice::open(tusb_desc_interface_t const *desc_itf, uint16_t max_len)
{
    TU_VERIFY(TUSB_CLASS_AUDIO == desc_itf->bInterfaceClass &&
                  AUDIO_SUBCLASS_CONTROL == desc_itf->bInterfaceSubClass,
              0);
    uint8_t const *p_desc = (uint8_t const *)desc_itf;
    uint8_t const *desc_end = p_desc + max_len;

    // the Audio Control interface's class specific descriptors
    p_desc = tu_desc_next(p_desc);
    while (tu_desc_in_bounds(p_desc, desc_end) && TUSB_DESC_CS_INTERFACE == tu_desc_type(p_desc))
    {
        p_desc = tu_desc_next(p_desc);
    }

    TU_VERIFY(tu_desc_in_bounds(p_desc, desc_end) && TUSB_DESC_INTERFACE == tu_desc_type(p_desc), 0);
    tusb_desc_interface_t const *desc_midi = (tusb_desc_interface_t const *)p_desc;
    TU_VERIFY(TUSB_CLASS_AUDIO == desc_midi->bInterfaceClass &&
                  AUDIO_SUBCLASS_MIDI_STREAMING == desc_midi->bInterfaceSubClass,
              0);
    p_desc = tu_desc_next(p_desc);

    // jacks, then each endpoint followed by its class specific endpoint descriptor
    uint8_t found_ep = 0;
    while (found_ep < desc_midi->bNumEndpoints && tu_desc_in_bounds(p_desc, desc_end))
    {
        if (TUSB_DESC_ENDPOINT == tu_desc_type(p_desc))
        {
            tusb_desc_endpoint_t const *desc_ep = (tusb_desc_endpoint_t const *)p_desc;
            TU_VERIFY(usbd_edpt_open(TUD_OPT_RHPORT, desc_ep), 0);
            if (tu_edpt_dir(desc_ep->bEndpointAddress) == TUSB_DIR_OUT)
            {
                TU_VERIFY(usbd_edpt_xfer(TUD_OPT_RHPORT, m_epout, m_epout_buf, sizeof(m_epout_buf), false), 0);
            }
            p_desc = tu_desc_next(p_desc);
            found_ep++;
        }
        p_desc = tu_desc_next(p_desc);
    }
    TU_VERIFY(found_ep == desc_midi->bNumEndpoints, 0);
    return (uint16_t)(p_desc - (uint8_t const *)desc_itf);
}

bool UsbMidiDevice::interrupt_xfer(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (tu_edpt_dir(ep_addr) == TUSB_DIR_IN)
    {
        // get the next batch going straight away rather than waiting for the next poll
        send_pending();
        return true;
    }
    TU_VERIFY(usbd_edpt_xfer(TUD_OPT_RHPORT, m_epout, m_epout_buf, sizeof(m_epout_buf), false));
    return true;
}

bool UsbMidiDevice::control_transfer(uint8_t stage, tusb_control_request_t const *request)
{
    // nothing on either interface takes class requests
    return false;
}
