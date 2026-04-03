
#include "tusb_option.h"
#include "usb/host/midi_host.h"
#include "class/midi/midi.h"
#include "class/audio/audio.h"

#define TU_LOG_DRV(...) TU_LOG(1, __VA_ARGS__)

typedef struct TU_ATTR_PACKED
{
  uint8_t bLength;            ///< Size of this descriptor in bytes.
  uint8_t bDescriptorType;    ///< Descriptor Type, must be Class-Specific
  uint8_t bDescriptorSubType; ///< Descriptor SubType
  uint8_t always_2;           /// always 0x2
  uint8_t num_cables_rx;      /// rx cable count
  uint8_t num_cables_tx;      /// tx cable count
} midi_desc_roland_header_t;

#define MIDI_CS_ROLAND_HEADER 0xF1

MidiHost::~MidiHost()
{
  TU_LOG_DRV("MIDI Host Interface destroyed\r\n");
    tu_edpt_stream_deinit(&ep_stream.rx);
    tu_edpt_stream_deinit(&ep_stream.tx);
}

MidiHost::MidiHost(uint8_t dev_addr, uint8_t interface, uint16_t id) : UsbHostInterface(dev_addr, interface, id)
{

  tu_memclr(&ep_stream, sizeof(ep_stream));
  tu_edpt_stream_init(&ep_stream.rx, true, false, false,
    ep_stream.rx_ff_buf, TUH_EPSIZE_BULK_MAX, m_ep_in_buf);
  tu_edpt_stream_init(&ep_stream.tx, true, true, false,
    ep_stream.tx_ff_buf, TUH_EPSIZE_BULK_MAX, m_ep_out_buf);
}

std::shared_ptr<UsbHostInterface> MidiHost::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *desc_itf, uint16_t max_len, uint16_t *out_len)
{
  // Some roland devices use vendor specific and have their own header
  TU_VERIFY(TUSB_CLASS_AUDIO == desc_itf->bInterfaceClass || TUSB_CLASS_VENDOR_SPECIFIC == desc_itf->bInterfaceClass, 0);
  bool roland = TUSB_CLASS_VENDOR_SPECIFIC == desc_itf->bInterfaceClass;
  uint8_t dev_addr = list->dev_addr();
  const uint8_t *desc_start = (const uint8_t *)desc_itf;
  const uint8_t *p_desc = desc_start;
  const uint8_t *desc_end = desc_start + max_len;

  if (AUDIO_SUBCLASS_CONTROL == desc_itf->bInterfaceSubClass)
  {
    TU_VERIFY(max_len > 2 * sizeof(tusb_desc_interface_t) + sizeof(midi10_desc_cs_ac_interface_t), 0);
    p_desc = tu_desc_next(p_desc);
    TU_VERIFY(tu_desc_type(p_desc) == TUSB_DESC_CS_INTERFACE &&
                  tu_desc_subtype(p_desc) == AUDIO10_CS_AC_INTERFACE_HEADER,
              0);

    p_desc = tu_desc_next(p_desc);
    desc_itf = (const tusb_desc_interface_t *)p_desc;
    // skip non-interface and non-midi streaming descriptors
    while (tu_desc_in_bounds(p_desc, desc_end) && (desc_itf->bDescriptorType != TUSB_DESC_INTERFACE ||
                                                   (desc_itf->bInterfaceClass == TUSB_CLASS_AUDIO &&
                                                    desc_itf->bInterfaceSubClass != AUDIO_SUBCLASS_MIDI_STREAMING)))
    {
      p_desc = tu_desc_next(p_desc);
      desc_itf = (const tusb_desc_interface_t *)p_desc;
    }
    TU_VERIFY(p_desc < desc_end, 0);
    TU_VERIFY(TUSB_CLASS_AUDIO == desc_itf->bInterfaceClass, 0);
  }
  TU_VERIFY(AUDIO_SUBCLASS_MIDI_STREAMING == desc_itf->bInterfaceSubClass, 0);

  TU_LOG_DRV("MIDI opening Interface %u (addr = %u)\r\n", desc_itf->bInterfaceNumber, dev_addr);

  auto intf = std::make_shared<MidiHost>(dev_addr, desc_itf->bInterfaceNumber, list->m_id);

  bool found_new_interface = false;
  do
  {
    p_desc = tu_desc_next(p_desc);
    if (!tu_desc_in_bounds(p_desc, desc_end))
    {
      break;
    }
    switch (tu_desc_type(p_desc))
    {
    case TUSB_DESC_INTERFACE:
      found_new_interface = true;
      break;

    case TUSB_DESC_CS_INTERFACE:
      switch (tu_desc_subtype(p_desc))
      {
      case MIDI_CS_INTERFACE_HEADER:
        TU_LOG_DRV("  Interface Header descriptor\r\n");
        break;

      case MIDI_CS_INTERFACE_IN_JACK:
      case MIDI_CS_INTERFACE_OUT_JACK:
      {
        TU_LOG_DRV("  Jack %s %s descriptor \r\n",
                   tu_desc_subtype(p_desc) == MIDI_CS_INTERFACE_IN_JACK ? "IN" : "OUT",
                   p_desc[3] == MIDI_JACK_EXTERNAL ? "External" : "Embedded");
        break;
      }

      case MIDI_CS_INTERFACE_ELEMENT:
        TU_LOG_DRV("  Element descriptor\r\n");
        break;

      case MIDI_CS_ROLAND_HEADER:
      {
        midi_desc_roland_header_t const *p_rl = (midi_desc_roland_header_t const *)p_desc;
        TU_LOG2("Found Roland Header %02x %02x %02x %02x\r\n", p_rl->bLength, p_rl->always_2, p_rl->num_cables_rx, p_rl->num_cables_tx);
        if (p_rl->bLength >= sizeof(midi_desc_roland_header_t) && p_rl->always_2 == 0x02)
        {
          TU_VERIFY(p_rl->num_cables_rx < 0x10 && p_rl->num_cables_tx < 0x10, nullptr);
          TU_LOG2("Found Roland Header\r\n");
          intf->rx_cable_count = (1 << p_rl->num_cables_rx) - 1;
          intf->tx_cable_count = (1 << p_rl->num_cables_tx) - 1;
          break;
        }
        TU_LOG_DRV("  Unknown CS Interface sub-type %u\r\n", tu_desc_subtype(p_desc));
        break;
      }
      default:
        TU_LOG_DRV("  Unknown CS Interface sub-type %u\r\n", tu_desc_subtype(p_desc));
        break;
      }
      break;

    case TUSB_DESC_ENDPOINT:
    {
      const tusb_desc_endpoint_t *p_ep = (const tusb_desc_endpoint_t *)p_desc;

      p_desc = tu_desc_next(p_desc); // next to CS endpoint
      TU_VERIFY(tu_desc_in_bounds(p_desc, desc_end), 0);
      tu_edpt_stream_t *ep_stream;
      if (roland)
      {
        if (tu_edpt_dir(p_ep->bEndpointAddress) == TUSB_DIR_OUT)
        {
          ep_stream = &intf->ep_stream.tx;
        }
        else
        {
          ep_stream = &intf->ep_stream.rx;
        }
      }
      else
      {
        const midi_desc_cs_endpoint_t *p_csep = (const midi_desc_cs_endpoint_t *)p_desc;

        TU_LOG_DRV("  Endpoint and CS_Endpoint descriptor %02x\r\n", p_ep->bEndpointAddress);
        if (tu_edpt_dir(p_ep->bEndpointAddress) == TUSB_DIR_OUT)
        {
          intf->tx_cable_count = p_csep->bNumEmbMIDIJack;
          ep_stream = &intf->ep_stream.tx;
          list->host_devices_by_endpoint_out[intf->m_ep_out] = intf;
        }
        else
        {
          intf->rx_cable_count = p_csep->bNumEmbMIDIJack;
          ep_stream = &intf->ep_stream.rx;
          list->host_devices_by_endpoint_in[intf->m_ep_in & (~0x80)] = intf;
        }
      }
      TU_ASSERT(tuh_edpt_open(dev_addr, p_ep), 0);
      tu_edpt_stream_open(ep_stream, dev_addr, p_ep, tu_edpt_packet_size(p_ep));
      tu_edpt_stream_clear(ep_stream);

      break;
    }

    default:
      break; // skip unknown descriptor
    }
  } while (!found_new_interface);

  *out_len = (uint16_t)((uintptr_t)p_desc - (uintptr_t)desc_start);
  return intf;
}

bool MidiHost::set_config()
{
  return true;
}

bool MidiHost::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
  tu_edpt_stream_t *ep_str_rx = &ep_stream.rx;
  tu_edpt_stream_t *ep_str_tx = &ep_stream.tx;
  if (ep_addr == ep_str_rx->ep_addr)
  {
    // receive new data, put it into FIFO and invoke callback if available
    // Note: some devices send back all zero packets even if there is no data ready
    if (xferred_bytes && !tu_mem_is_zero(ep_str_rx->ep_buf, xferred_bytes))
    {
      tu_edpt_stream_read_xfer_complete(ep_str_rx, xferred_bytes);
      // tuh_midi_rx_cb(idx, xferred_bytes);
    }

    tu_edpt_stream_read_xfer(ep_str_rx); // prepare for next transfer
  }
  else if (ep_addr == ep_str_tx->ep_addr)
  {
    // tuh_midi_tx_cb(idx, xferred_bytes);

    if (0 == tu_edpt_stream_write_xfer(ep_str_tx))
    {
      // If there is no data left, a ZLP should be sent if
      // xferred_bytes is multiple of EP size and not zero
      tu_edpt_stream_write_zlp_if_needed(ep_str_tx, xferred_bytes);
    }
  }
  return true;
}

bool MidiHost::tick_digital(UsbButtonType type)
{
  return false;
}
uint16_t MidiHost::tick_analog(UsbAxisType type)
{
  return 0;
}