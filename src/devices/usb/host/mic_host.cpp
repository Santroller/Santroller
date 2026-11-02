#include "tusb_option.h"
#include "devices/usb/host/mic_host.h"
#include "devices/mic_stream.hpp"
#include "class/audio/audio.h"
#include "pio_usb_configuration.h"

#define TU_LOG_DRV(...) TU_LOG(1, __VA_ARGS__)

// PIO-USB's receive buffer holds SYNC, PID and CRC around the packet
static constexpr uint16_t MAX_PACKET_SIZE = PIO_USB_RX_BUFFER_SIZE - 4;

MicHost::~MicHost()
{
  TU_LOG_DRV("Mic Host Interface destroyed\r\n");
}

MicHost::MicHost(uint8_t dev_addr, uint8_t interface, uint16_t id) : UsbHostInterface(dev_addr, interface, id)
{
  m_subtype = SubType_Microphone;
}

std::shared_ptr<UsbHostInterface> MicHost::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *desc_itf, uint16_t max_len, uint16_t *out_len)
{
  // UAC2 devices mostly fall back to UAC1 at full speed, so only UAC1 is handled
  TU_VERIFY(TUSB_CLASS_AUDIO == desc_itf->bInterfaceClass &&
                AUDIO_SUBCLASS_CONTROL == desc_itf->bInterfaceSubClass &&
                AUDIO_INT_PROTOCOL_CODE_V1 == desc_itf->bInterfaceProtocol,
            nullptr);
  uint8_t const *desc_start = (uint8_t const *)desc_itf;
  uint8_t const *desc_end = desc_start + max_len;
  uint8_t const *p_desc = tu_desc_next(desc_start);

  auto intf = std::make_shared<MicHost>(list->dev_addr(), desc_itf->bInterfaceNumber, list->m_id);
  uint8_t last_itf = desc_itf->bInterfaceNumber;
  bool found = false;

  // The alt setting being parsed, its endpoint is a candidate once its format has been seen
  tusb_desc_interface_t const *alt = nullptr;
  bool pcm = false;
  uint8_t channels = 0;
  uint8_t subframe = 0;
  uint8_t const *format = nullptr;

  // a zero length descriptor would make tu_desc_next() return the same pointer forever
  while (tu_desc_in_bounds(p_desc, desc_end) && tu_desc_len(p_desc) != 0)
  {
    uint8_t const type = tu_desc_type(p_desc);
    if (type == TUSB_DESC_INTERFACE_ASSOCIATION)
    {
      break;
    }
    if (type == TUSB_DESC_INTERFACE)
    {
      tusb_desc_interface_t const *itf = (tusb_desc_interface_t const *)p_desc;
      // MIDI streaming after the control interface belongs to MidiHost
      TU_VERIFY(!(itf->bInterfaceClass == TUSB_CLASS_AUDIO && itf->bInterfaceSubClass == AUDIO_SUBCLASS_MIDI_STREAMING), nullptr);
      if (itf->bInterfaceClass != TUSB_CLASS_AUDIO || itf->bInterfaceSubClass != AUDIO_SUBCLASS_STREAMING)
      {
        // the next function, eg a headset's HID interface
        break;
      }
      alt = itf;
      pcm = false;
      channels = 0;
      subframe = 0;
      format = nullptr;
      last_itf = itf->bInterfaceNumber;
    }
    else if (alt && type == TUSB_DESC_CS_INTERFACE && tu_desc_len(p_desc) >= 7)
    {
      if (tu_desc_subtype(p_desc) == AUDIO10_CS_AS_INTERFACE_AS_GENERAL)
      {
        pcm = tu_le16toh(tu_unaligned_read16(p_desc + 5)) == AUDIO10_DATA_FORMAT_TYPE_I_PCM;
      }
      else if (tu_desc_subtype(p_desc) == AUDIO10_CS_AS_INTERFACE_FORMAT_TYPE && tu_desc_len(p_desc) >= 8 &&
               p_desc[3] == AUDIO10_FORMAT_TYPE_I)
      {
        channels = p_desc[4];
        subframe = p_desc[5];
        format = p_desc;
      }
    }
    else if (alt && type == TUSB_DESC_ENDPOINT)
    {
      tusb_desc_endpoint_t const *ep = (tusb_desc_endpoint_t const *)p_desc;
      bool const usable = pcm && format && subframe == 2 && (channels == 1 || channels == 2) &&
                          ep->bmAttributes.xfer == TUSB_XFER_ISOCHRONOUS &&
                          ep->bmAttributes.usage == 0 &&
                          tu_edpt_dir(ep->bEndpointAddress) == TUSB_DIR_IN &&
                          tu_edpt_packet_size(ep) <= MAX_PACKET_SIZE;
      // mono over stereo, otherwise the first that works
      if (usable && (!found || (channels == 1 && intf->m_channels != 1)))
      {
        found = true;
        intf->m_as_itf = alt->bInterfaceNumber;
        intf->m_alt = alt->bAlternateSetting;
        intf->m_channels = channels;
        intf->m_subframe = subframe;
        intf->m_ep = *ep;
        uint8_t const freq_type = format[7];
        uint8_t const avail = (tu_desc_len(format) - 8) / 3;
        intf->m_continuous = freq_type == 0;
        intf->m_rate_count = freq_type == 0 ? TU_MIN(avail, 2) : TU_MIN(TU_MIN(freq_type, avail), MAX_RATES);
        for (uint8_t i = 0; i < intf->m_rate_count; i++)
        {
          uint8_t const *f = format + 8 + i * 3;
          intf->m_rates[i] = f[0] | (f[1] << 8) | (f[2] << 16);
        }
        // the class specific endpoint descriptor says whether the rate can be set
        uint8_t const *cs_ep = tu_desc_next(p_desc);
        intf->m_rate_control = tu_desc_in_bounds(cs_ep, desc_end) && tu_desc_len(cs_ep) >= 4 &&
                               tu_desc_type(cs_ep) == TUSB_DESC_CS_ENDPOINT &&
                               (cs_ep[3] & AUDIO10_CS_AS_ISO_DATA_EP_ATT_SAMPLING_FRQ);
      }
    }
    p_desc = tu_desc_next(p_desc);
  }
  TU_VERIFY(found && intf->m_rate_count, nullptr);

  TU_LOG_DRV("Mic opening Interface %u alt %u (addr = %u)\r\n", intf->m_as_itf, intf->m_alt, intf->dev_addr());
  // set_config() completes from the last interface taken, so it isn't called for the streaming ones
  intf->m_interface = last_itf;
  list->host_devices_by_endpoint_in[intf->m_ep.bEndpointAddress & (~0x80)] = intf;
  usb_host_add_assignable_interface(intf);
  *out_len = (uint16_t)(p_desc - desc_start);
  return intf;
}

bool MicHost::pick_rate()
{
  // 48kHz is what most things want, then the highest that is still a sensible rate for a voice
  static constexpr uint32_t PREFERRED[] = {48000, 44100, 32000, 24000, 22050, 16000};
  uint16_t const packet_size = tu_edpt_packet_size(&m_ep);
  auto fits = [&](uint32_t rate)
  {
    // a packet can carry one extra sample when the device's clock runs fast
    return (rate / 1000 + 1) * m_channels * m_subframe <= packet_size;
  };
  m_rate = 0;
  if (m_continuous)
  {
    for (uint32_t rate : PREFERRED)
    {
      if (rate >= m_rates[0] && rate <= m_rates[m_rate_count - 1] && fits(rate))
      {
        m_rate = rate;
        return true;
      }
    }
    m_rate = m_rates[0];
    return fits(m_rate);
  }
  for (uint32_t rate : PREFERRED)
  {
    for (uint8_t i = 0; i < m_rate_count; i++)
    {
      if (m_rates[i] == rate && fits(rate))
      {
        m_rate = rate;
        return true;
      }
    }
  }
  for (uint8_t i = 0; i < m_rate_count; i++)
  {
    if (fits(m_rates[i]) && m_rates[i] > m_rate)
    {
      m_rate = m_rates[i];
    }
  }
  return m_rate != 0;
}

bool MicHost::set_config()
{
  if (pick_rate())
  {
    bool ok = true;
    tusb_control_request_t set_interface = {};
    set_interface.bmRequestType = 0x01; // standard, interface, out
    set_interface.bRequest = TUSB_REQ_SET_INTERFACE;
    set_interface.wValue = m_alt;
    set_interface.wIndex = m_as_itf;
    send_ctrl_xfer(set_interface, nullptr, &ok);
    if (ok && m_rate_control)
    {
      CFG_TUSB_MEM_ALIGN static uint8_t rate[3];
      rate[0] = m_rate;
      rate[1] = m_rate >> 8;
      rate[2] = m_rate >> 16;
      tusb_control_request_t set_rate = {};
      set_rate.bmRequestType = 0x22; // class, endpoint, out
      set_rate.bRequest = AUDIO10_CS_REQ_SET_CUR;
      set_rate.wValue = AUDIO10_EP_CTRL_SAMPLING_FREQ << 8;
      set_rate.wIndex = m_ep.bEndpointAddress;
      set_rate.wLength = sizeof(rate);
      // devices that only have the one rate sometimes stall this, which is fine
      send_ctrl_xfer(set_rate, rate, nullptr);
    }
    TU_LOG_DRV("Mic streaming at %lu Hz, %u channels (%s)\r\n", m_rate, m_channels, ok ? "ok" : "set interface failed");
    if (ok && tuh_edpt_open(m_dev_addr, &m_ep))
    {
      MicStream::set_source_rate(m_rate);
      queue_xfer();
    }
  }
  UsbHostInterface::set_config();
  return true;
}

void MicHost::queue_xfer()
{
  uint8_t const ep = m_ep.bEndpointAddress;
  if (!usbh_edpt_claim(m_dev_addr, ep))
  {
    return;
  }
  if (!usbh_edpt_xfer(m_dev_addr, ep, m_buf, sizeof(m_buf)))
  {
    usbh_edpt_release(m_dev_addr, ep);
  }
}

bool MicHost::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
  if (result == XFER_RESULT_SUCCESS)
  {
    uint32_t const frame = m_channels * m_subframe;
    uint32_t off = 0;
    while (off + 2 <= xferred_bytes)
    {
      uint16_t const len = m_buf[off] | (m_buf[off + 1] << 8);
      off += 2;
      if (len == 0xFFFF)
      {
        MicStream::gap();
        continue;
      }
      if (off + len > xferred_bytes)
      {
        break;
      }
      // the first channel of 16 bit little endian samples
      size_t count = 0;
      for (uint32_t i = 0; i + frame <= len && count < TU_ARRAY_SIZE(m_samples); i += frame)
      {
        m_samples[count++] = (int16_t)(m_buf[off + i] | (m_buf[off + i + 1] << 8));
      }
      MicStream::push(m_samples, count);
      off += len;
    }
  }
  // A failed transfer means the ring went to another endpoint or the device went quiet. Queueing
  // another starts it again.
  if (still_connected)
  {
    queue_xfer();
  }
  return true;
}

void MicHost::disconnect()
{
  MicStream::set_source_rate(0);
  UsbHostInterface::disconnect();
}
