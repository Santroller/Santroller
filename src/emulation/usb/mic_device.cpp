#include "emulation/usb/mic_device.h"
#include <string.h>
#include "managers/profile_manager.hpp"
#include "managers/config_manager.hpp"
#include "main.hpp"
#include "utils.h"
#include "emulation/usb/usb_devices.h"
#include "devices/mic_stream.hpp"
#include "device/usbd.h"
#include "device/usbd_pvt.h"
#include "class/audio/audio.h"

// The unit the mute and volume requests go to (see TUD_AUDIO10_MIC_ONE_CH_DESCRIPTOR)
static constexpr uint8_t FEATURE_UNIT_ID = 2;
static constexpr uint32_t RATES[] = {8000, 11025, 16000, 22050, 32000, 44100, 48000};
#define USB_MIC_RATES 8000, 11025, 16000, 22050, 32000, 44100, 48000
// Volume range offered to the host, in 1/256 dB. The Logitech microphone's, as Wii games set it
static constexpr int16_t VOLUME_MIN = (int16_t)0x8001;
static constexpr int16_t VOLUME_MAX = 0x0800;
static constexpr int16_t VOLUME_RES = 0x0088;
// The Logitech USB Microphone's audio endpoint
static constexpr uint8_t LOGITECH_MIC_EP = 0x84;

UsbMicDevice::UsbMicDevice()
{
}

void UsbMicDevice::initialize()
{
    m_epin = next_epin();
    m_strid = next_strid();
    m_strid_logitech = next_strid();
    next_strid();
    m_ep = m_epin;
    ProfileManager::instance().map_usb_instance_epin(m_epin, interface_id);
    m_streaming = false;
}

bool UsbMicDevice::alone() const
{
    // the config and security interfaces have no profiles
    bool alone = true;
    ProfileManager::instance().for_each_usb_instance([this, &alone](const auto &instance)
                                                     { alone &= instance.get() == this || instance->profiles.empty(); });
    return alone;
}

bool UsbMicDevice::standalone() const
{
    // In HID mode the config and security interfaces stay, for the config tool and to find a 360
    return ConfigManager::instance().get_current_mode() != ModeHid && alone();
}

bool UsbMicDevice::s_alone_in_hid = false;

void UsbMicDevice::process(bool full_poll, bool send_events)
{
    // The audio goes out as each packet completes. Without a controller to work out the console, it is
    // done here: a PC reads the strings and keeps it in HID mode, a console that doesn't (eg a Wii) gets
    // the Logitech microphone on its own. A 360 and an Xbox One are found by the config and security
    // interfaces, like they are for a controller.
    if (ConfigManager::instance().get_current_mode() != ModeHid)
    {
        return;
    }
    s_alone_in_hid = alone();
    auto &detection = UsbDetectionState::instance();
    if (s_alone_in_hid && !mode_recently_changed() && detection.should_infer_console(millis()))
    {
        ConfigManager::instance().host_detected(detection.seen_hid_descriptor_read() ? ModeSwitch : ModeWiiRb);
    }
}

size_t UsbMicDevice::compatible_section_descriptor(uint8_t *dest, size_t remaining)
{
    return 0;
}

size_t UsbMicDevice::config_descriptor(uint8_t *dest, size_t remaining)
{
    const bool logitech = standalone();
    m_ep = logitech ? LOGITECH_MIC_EP : m_epin;
    uint8_t desc[] = {TUD_AUDIO10_MIC_ONE_CH_DESCRIPTOR(interface_id, (uint8_t)(logitech ? 0 : m_strid), 2, 16, m_ep, USB_MIC_EP_SIZE, USB_MIC_RATES)};
    if (logitech)
    {
        // It is the whole device, so nothing else has the Logitech microphone's endpoint. That endpoint is
        // synchronous too: the attributes are the 4th of the endpoint descriptor's 9 bytes, which comes
        // right before the class specific one at the end
        ProfileManager::instance().map_usb_instance_epin(m_ep, interface_id);
        desc[sizeof(desc) - TUD_AUDIO10_DESC_CS_AS_ISO_EP_LEN - TUD_AUDIO10_DESC_STD_AS_ISO_EP_LEN + 3] =
            TUSB_XFER_ISOCHRONOUS | TUSB_ISO_EP_ATT_SYNCHRONOUS;
    }
    assert(sizeof(desc) <= remaining);
    memcpy(dest, desc, sizeof(desc));
    return sizeof(desc);
}

size_t UsbMicDevice::device_name(uint8_t idx, char *desc)
{
    if (idx == m_strid_logitech || idx == m_strid_logitech + 1)
    {
        const char *name = idx == m_strid_logitech ? "Logitech" : "Logitech USB Microphone";
        strcpy(desc, name);
        return strlen(name) + 1;
    }
    if (idx == m_strid)
    {
        memcpy(desc, profiles[0]->name, sizeof(profiles[0]->name));
        return sizeof(profiles[0]->name);
    }
    return 0;
}

// Some games only take a Logitech microphone (most on the Wii, and some on the PS3, which is never told
// apart from a PC without a controller), so on its own it always looks like one. The config tool looks
// for it as well as a Santroller.
void UsbMicDevice::device_descriptor(tusb_desc_device_t *desc)
{
    if (!alone())
    {
        return;
    }
    desc->idVendor = LOGITECH_VID;
    desc->idProduct = LOGITECH_MIC_PID;
    desc->bcdDevice = 0x0001;
    desc->iManufacturer = m_strid_logitech;
    desc->iProduct = m_strid_logitech + 1;
    if (standalone())
    {
        desc->iSerialNumber = 0;
    }
}

// Takes both interfaces, so tinyusb hands this device the requests for either of them
uint16_t UsbMicDevice::open(tusb_desc_interface_t const *desc_itf, uint16_t max_len)
{
    TU_VERIFY(TUSB_CLASS_AUDIO == desc_itf->bInterfaceClass &&
                  AUDIO_SUBCLASS_CONTROL == desc_itf->bInterfaceSubClass,
              0);
    uint8_t const *p_desc = (uint8_t const *)desc_itf;
    uint8_t const *desc_end = p_desc + max_len;
    p_desc = tu_desc_next(p_desc);
    while (tu_desc_in_bounds(p_desc, desc_end) && tu_desc_len(p_desc) != 0)
    {
        if (TUSB_DESC_INTERFACE == tu_desc_type(p_desc) &&
            ((tusb_desc_interface_t const *)p_desc)->bInterfaceNumber != interface_id + 1)
        {
            break;
        }
        if (TUSB_DESC_ENDPOINT == tu_desc_type(p_desc))
        {
            // Only alt setting 1 has it, so it is enabled when the host picks that
            tusb_desc_endpoint_t const *desc_ep = (tusb_desc_endpoint_t const *)p_desc;
            TU_VERIFY(usbd_edpt_iso_alloc(TUD_OPT_RHPORT, desc_ep->bEndpointAddress, tu_edpt_packet_size(desc_ep)), 0);
        }
        p_desc = tu_desc_next(p_desc);
    }
    m_streaming = false;
    return (uint16_t)(p_desc - (uint8_t const *)desc_itf);
}

void UsbMicDevice::set_streaming(bool streaming)
{
    m_streaming = streaming;
    if (!streaming)
    {
        return;
    }
    tusb_desc_endpoint_t const desc_ep = {
        .bLength = sizeof(tusb_desc_endpoint_t),
        .bDescriptorType = TUSB_DESC_ENDPOINT,
        .bEndpointAddress = m_ep,
        .bmAttributes = {.xfer = TUSB_XFER_ISOCHRONOUS, .sync = (uint8_t)(m_ep == LOGITECH_MIC_EP ? 3 : 1)},
        .wMaxPacketSize = USB_MIC_EP_SIZE,
        .bInterval = 1,
    };
    usbd_edpt_iso_activate(TUD_OPT_RHPORT, &desc_ep);
    m_remainder = 0;
    queue_packet();
}

void UsbMicDevice::set_volume(int16_t volume)
{
    m_volume = volume < VOLUME_MIN ? VOLUME_MIN : volume > VOLUME_MAX ? VOLUME_MAX : volume;
    // 2^(dB / 6.0206), as a whole power of two and a quadratic fit of the rest (within 0.2%)
    int32_t const log2_q8 = ((int32_t)m_volume * 2721) >> 14;
    int32_t const whole = log2_q8 >> 8;
    int32_t const frac = log2_q8 & 0xFF;
    int32_t const gain = 4096 + ((2689 * frac) >> 8) + ((1407 * frac * frac) >> 16);
    m_gain = whole >= 0 ? gain << whole : whole > -13 ? gain >> -whole : 0;
}

void UsbMicDevice::queue_packet()
{
    uint8_t const ep = m_ep;
    if (!m_streaming || !usbd_edpt_claim(TUD_OPT_RHPORT, ep))
    {
        return;
    }
    m_remainder += m_rate;
    size_t count = m_remainder / 1000;
    m_remainder %= 1000;
    MicStream::read(m_packet, count, m_rate);
    for (size_t i = 0; i < count && m_gain != 4096; i++)
    {
        int32_t const sample = (m_packet[i] * m_gain) >> 12;
        m_packet[i] = (int16_t)(sample > INT16_MAX ? INT16_MAX : sample < INT16_MIN ? INT16_MIN : sample);
    }
    if (m_mute)
    {
        memset(m_packet, 0, count * sizeof(*m_packet));
    }
    if (!usbd_edpt_xfer(TUD_OPT_RHPORT, ep, (uint8_t *)m_packet, count * sizeof(*m_packet), false))
    {
        usbd_edpt_release(TUD_OPT_RHPORT, ep);
    }
}

bool UsbMicDevice::interrupt_xfer(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    // a packet went out (or was missed), get the next one ready for the next frame
    queue_packet();
    return true;
}

bool UsbMicDevice::control_transfer(uint8_t stage, tusb_control_request_t const *request)
{
    if (request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_INTERFACE)
    {
        return interface_request(stage, request);
    }
    if (request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_ENDPOINT)
    {
        return endpoint_request(stage, request);
    }
    return false;
}

bool UsbMicDevice::interface_request(uint8_t stage, tusb_control_request_t const *request)
{
    uint8_t const itf = tu_u16_low(request->wIndex);
    if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_STANDARD)
    {
        TU_VERIFY(itf == interface_id + 1);
        if (request->bRequest == TUSB_REQ_SET_INTERFACE)
        {
            uint8_t const alt = tu_u16_low(request->wValue);
            TU_VERIFY(alt <= 1);
            if (stage == CONTROL_STAGE_SETUP)
            {
                set_streaming(alt == 1);
                tud_control_status(TUD_OPT_RHPORT, request);
            }
            return true;
        }
        if (request->bRequest == TUSB_REQ_GET_INTERFACE)
        {
            if (stage == CONTROL_STAGE_SETUP)
            {
                m_ctrl_buf[0] = m_streaming;
                tud_control_xfer(TUD_OPT_RHPORT, request, m_ctrl_buf, 1);
            }
            return true;
        }
        return false;
    }

    // mute and volume, on the feature unit
    TU_VERIFY(request->bmRequestType_bit.type == TUSB_REQ_TYPE_CLASS && itf == interface_id &&
              tu_u16_high(request->wIndex) == FEATURE_UNIT_ID);
    uint8_t const control = tu_u16_high(request->wValue);
    TU_VERIFY(control == AUDIO10_FU_CTRL_MUTE || control == AUDIO10_FU_CTRL_VOLUME);
    uint16_t const len = control == AUDIO10_FU_CTRL_MUTE ? 1 : 2;
    if (request->bmRequestType_bit.direction == TUSB_DIR_OUT)
    {
        TU_VERIFY(request->bRequest == AUDIO10_CS_REQ_SET_CUR && request->wLength >= len);
        if (stage == CONTROL_STAGE_SETUP)
        {
            return tud_control_xfer(TUD_OPT_RHPORT, request, m_ctrl_buf, len);
        }
        if (stage == CONTROL_STAGE_DATA)
        {
            if (control == AUDIO10_FU_CTRL_MUTE)
            {
                m_mute = m_ctrl_buf[0];
            }
            else
            {
                set_volume((int16_t)tu_unaligned_read16(m_ctrl_buf));
            }
        }
        return true;
    }
    if (stage != CONTROL_STAGE_SETUP)
    {
        return true;
    }
    if (control == AUDIO10_FU_CTRL_MUTE)
    {
        TU_VERIFY(request->bRequest == AUDIO10_CS_REQ_GET_CUR);
        m_ctrl_buf[0] = m_mute;
        return tud_control_xfer(TUD_OPT_RHPORT, request, m_ctrl_buf, 1);
    }
    int16_t value;
    switch (request->bRequest)
    {
    case AUDIO10_CS_REQ_GET_CUR:
        value = m_volume;
        break;
    case AUDIO10_CS_REQ_GET_MIN:
        value = VOLUME_MIN;
        break;
    case AUDIO10_CS_REQ_GET_MAX:
        value = VOLUME_MAX;
        break;
    case AUDIO10_CS_REQ_GET_RES:
        value = VOLUME_RES;
        break;
    default:
        return false;
    }
    tu_unaligned_write16(m_ctrl_buf, (uint16_t)value);
    return tud_control_xfer(TUD_OPT_RHPORT, request, m_ctrl_buf, 2);
}

bool UsbMicDevice::endpoint_request(uint8_t stage, tusb_control_request_t const *request)
{
    // the sampling frequency, the only endpoint control
    TU_VERIFY(request->bmRequestType_bit.type == TUSB_REQ_TYPE_CLASS &&
              tu_u16_high(request->wValue) == AUDIO10_EP_CTRL_SAMPLING_FREQ);
    if (request->bmRequestType_bit.direction == TUSB_DIR_OUT)
    {
        TU_VERIFY(request->bRequest == AUDIO10_CS_REQ_SET_CUR && request->wLength >= 3);
        if (stage == CONTROL_STAGE_SETUP)
        {
            return tud_control_xfer(TUD_OPT_RHPORT, request, m_ctrl_buf, 3);
        }
        if (stage == CONTROL_STAGE_DATA)
        {
            uint32_t const rate = m_ctrl_buf[0] | (m_ctrl_buf[1] << 8) | (m_ctrl_buf[2] << 16);
            for (uint32_t supported : RATES)
            {
                if (rate == supported)
                {
                    m_rate = rate;
                    m_remainder = 0;
                }
            }
        }
        return true;
    }
    TU_VERIFY(request->bRequest == AUDIO10_CS_REQ_GET_CUR);
    if (stage == CONTROL_STAGE_SETUP)
    {
        m_ctrl_buf[0] = m_rate;
        m_ctrl_buf[1] = m_rate >> 8;
        m_ctrl_buf[2] = m_rate >> 16;
        return tud_control_xfer(TUD_OPT_RHPORT, request, m_ctrl_buf, 3);
    }
    return true;
}
