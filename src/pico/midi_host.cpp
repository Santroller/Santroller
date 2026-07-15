/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * This file is part of the TinyUSB stack.
 */

#include "tusb_option.h"

#if (CFG_TUH_ENABLED && CFG_TUH_MIDI_CUSTOM)

#include "host/usbh.h"
#include "host/usbh_pvt.h"

#include "midi_host.h"
#include "shared_main.h"
#include <stdio.h>

// Level where CFG_TUSB_DEBUG must be at least for this driver is logged
#ifndef CFG_TUH_MIDI_CUSTOM_LOG_LEVEL
#define CFG_TUH_MIDI_CUSTOM_LOG_LEVEL 0
#endif

#define TU_LOG_DRV(...) TU_LOG(CFG_TUH_MIDI_CUSTOM_LOG_LEVEL, __VA_ARGS__)

#define MIDI_CS_ROLAND_HEADER 0xF1
//--------------------------------------------------------------------+
// Weak stubs: invoked if no strong implementation is available
//--------------------------------------------------------------------+
TU_ATTR_WEAK void tuh_midi_descriptor_cb(uint8_t idx, const tuh_midi_descriptor_cb_t *desc_cb_data)
{
  (void)idx;
  (void)desc_cb_data;
}
TU_ATTR_WEAK void tuh_midi_mount_cb(uint8_t idx, const tuh_midi_mount_cb_t *mount_cb_data)
{
  (void)idx;
  (void)mount_cb_data;
}
TU_ATTR_WEAK void tuh_midi_umount_cb(uint8_t idx) { (void)idx; }
TU_ATTR_WEAK void tuh_midi_rx_cb(uint8_t idx, uint32_t xferred_bytes)
{
  (void)idx;
  (void)xferred_bytes;
}
TU_ATTR_WEAK void tuh_midi_tx_cb(uint8_t idx, uint32_t xferred_bytes)
{
  (void)idx;
  (void)xferred_bytes;
}

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF
//--------------------------------------------------------------------+

typedef struct
{
  uint8_t daddr;
  uint8_t bInterfaceNumber; // interface number of MIDI streaming
  uint8_t iInterface;
  uint8_t itf_count; // number of interfaces including Audio Control + MIDI streaming

  uint8_t rx_cable_count; // IN endpoint CS descriptor bNumEmbMIDIJack value
  uint8_t tx_cable_count; // OUT endpoint CS descriptor bNumEmbMIDIJack value

#if CFG_TUH_MIDI_CUSTOM_STREAM_API
  // For Stream read()/write() API
  // Messages are always 4 bytes long, queue them for reading and writing so the
  // callers can use the Stream interface with single-byte read/write calls.
  midi_driver_stream_t stream_write;
  midi_driver_stream_t stream_read;
#endif

  bool mounted;
} midih_interface_t;

typedef struct
{
  TUH_EPBUF_DEF(tx, TUH_EPSIZE_BULK_MAX);
  TUH_EPBUF_DEF(rx, TUH_EPSIZE_BULK_MAX);
} midih_epbuf_t;

/// MIDI In Jack Descriptor
typedef struct TU_ATTR_PACKED
{
  uint8_t bLength;            ///< Size of this descriptor in bytes.
  uint8_t bDescriptorType;    ///< Descriptor Type, must be Class-Specific
  uint8_t bDescriptorSubType; ///< Descriptor SubType
  uint8_t always_2;           /// always 0x2
  uint8_t num_cables_rx;      /// rx cable count
  uint8_t num_cables_tx;      /// tx cable count
} midi_desc_roland_header_t;
static midih_interface_t _midi_host[CFG_TUH_MIDI_CUSTOM];
CFG_TUH_MEM_SECTION static midih_epbuf_t _midi_epbuf[CFG_TUH_MIDI_CUSTOM];

//--------------------------------------------------------------------+
// Helper
//--------------------------------------------------------------------+
TU_ATTR_ALWAYS_INLINE static inline uint8_t find_new_midi_index(void)
{
  for (uint8_t idx = 0; idx < CFG_TUH_MIDI_CUSTOM; idx++)
  {
    if (_midi_host[idx].daddr == 0)
    {
      return idx;
    }
  }
  return TUSB_INDEX_INVALID_8;
}

static inline uint8_t get_idx_by_ep_addr(uint8_t daddr, uint8_t ep_addr)
{
  for (uint8_t idx = 0; idx < CFG_TUH_MIDI_CUSTOM; idx++)
  {
    const midih_interface_t *p_midi = &_midi_host[idx];
    if ((p_midi->daddr == daddr) &&
        (ep_addr == usbMidi.ep_stream.rx.ep_addr || ep_addr == usbMidi.ep_stream.tx.ep_addr))
    {
      return idx;
    }
  }
  return TUSB_INDEX_INVALID_8;
}

//--------------------------------------------------------------------+
// USBH API
//--------------------------------------------------------------------+
bool midih_init(void)
{
  tu_memclr(&_midi_host, sizeof(_midi_host));
  return true;
}

bool midih_deinit(void)
{
  // for (size_t i = 0; i < CFG_TUH_MIDI_CUSTOM; i++) {
  //   midih_interface_t* p_midi = &_midi_host[i];
  // tu_edpt_stream_deinit(&usbMidi.ep_stream.rx);
  // tu_edpt_stream_deinit(&usbMidi.ep_stream.tx);
  // }
  return true;
}

void midih_close(uint8_t daddr)
{
  for (uint8_t idx = 0; idx < CFG_TUH_MIDI_CUSTOM; idx++)
  {
    midih_interface_t *p_midi = &_midi_host[idx];
    if (p_midi->daddr == daddr)
    {
      TU_LOG_DRV("  MIDI close addr = %u index = %u\r\n", daddr, idx);
      tuh_midi_umount_cb(idx);

      p_midi->bInterfaceNumber = 0;
      p_midi->rx_cable_count = 0;
      p_midi->tx_cable_count = 0;
      p_midi->daddr = 0;
      p_midi->mounted = false;
#if CFG_TUH_MIDI_CUSTOM_STREAM_API
      tu_memclr(&p_midi->stream_read, sizeof(p_midi->stream_read));
      tu_memclr(&p_midi->stream_write, sizeof(p_midi->stream_write));
#endif

      tu_edpt_stream_close(&usbMidi.ep_stream.rx);
      tu_edpt_stream_close(&usbMidi.ep_stream.tx);
    }
  }
}

bool midih_xfer_cb(uint8_t dev_addr, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
  (void)result;
  const uint8_t idx = get_idx_by_ep_addr(dev_addr, ep_addr);
  TU_VERIFY(idx < CFG_TUH_MIDI_CUSTOM);
  midih_interface_t *p_midi = &_midi_host[idx];
  tu_edpt_stream_t *ep_str_rx = &usbMidi.ep_stream.rx;
  tu_edpt_stream_t *ep_str_tx = &usbMidi.ep_stream.tx;

  if (ep_addr == ep_str_rx->ep_addr)
  {
    // receive new data, put it into FIFO and invoke callback if available
    // Note: some devices send back all zero packets even if there is no data ready
    if (xferred_bytes && !tu_mem_is_zero(ep_str_rx->ep_buf, xferred_bytes))
    {
      tu_edpt_stream_read_xfer_complete(ep_str_rx, xferred_bytes);
      tuh_midi_rx_cb(idx, xferred_bytes);
    }

    tu_edpt_stream_read_xfer(ep_str_rx); // prepare for next transfer
  }
  else if (ep_addr == ep_str_tx->ep_addr)
  {
    tuh_midi_tx_cb(idx, xferred_bytes);

    if (0 == tu_edpt_stream_write_xfer(ep_str_tx))
    {
      // If there is no data left, a ZLP should be sent if
      // xferred_bytes is multiple of EP size and not zero
      tu_edpt_stream_write_zlp_if_needed(ep_str_tx, xferred_bytes);
    }
  }

  return true;
}

//--------------------------------------------------------------------+
// Enumeration
//--------------------------------------------------------------------+
uint16_t midih_open(uint8_t rhport, uint8_t dev_addr, const tusb_desc_interface_t *desc_itf, uint16_t max_len)
{
  (void)rhport;
  // Some roland devices use vendor specific and have their own header
  TU_VERIFY(TUSB_CLASS_AUDIO == desc_itf->bInterfaceClass || TUSB_CLASS_VENDOR_SPECIFIC == desc_itf->bInterfaceClass, 0);
  bool roland = TUSB_CLASS_VENDOR_SPECIFIC == desc_itf->bInterfaceClass;
  const uint8_t *desc_start = (const uint8_t *)desc_itf;
  const uint8_t *p_desc = desc_start;
  const uint8_t *desc_end = desc_start + max_len;

  const uint8_t idx = find_new_midi_index();
  TU_VERIFY(idx < CFG_TUH_MIDI_CUSTOM, 0);
  midih_interface_t *p_midi = &_midi_host[idx];
  p_midi->itf_count = 0;

  tuh_midi_descriptor_cb_t desc_cb = {0};
  desc_cb.jack_num = 0;

  // There can be just a MIDI or an Audio + MIDI interface
  // - If there is Audio Control Interface + Audio Header descriptor, then skip it.
  // - If there is an Audio Control Interface + Audio Streaming Interface, then ignore the Audio Streaming Interface.
  // Future:
  // Note that if this driver is used with an USB Audio Streaming host driver,
  // then call that driver first. If the MIDI interface comes before the
  // audio streaming interface, then the audio driver will have to call this
  // driver after parsing the audio control interface and then resume parsing
  // the streaming audio interface.
  if (AUDIO_SUBCLASS_CONTROL == desc_itf->bInterfaceSubClass)
  {
    TU_VERIFY(max_len > 2 * sizeof(tusb_desc_interface_t) + sizeof(midi10_desc_cs_ac_interface_t), 0);
    p_desc = tu_desc_next(p_desc);
    TU_VERIFY(tu_desc_type(p_desc) == TUSB_DESC_CS_INTERFACE &&
                  tu_desc_subtype(p_desc) == AUDIO10_CS_AC_INTERFACE_HEADER,
              0);
    desc_cb.desc_audio_control = desc_itf;

    p_desc = tu_desc_next(p_desc);
    desc_itf = (const tusb_desc_interface_t *)p_desc;
    p_midi->itf_count = 1;
    // skip non-interface and non-midi streaming descriptors
    while (tu_desc_in_bounds(p_desc, desc_end) && (desc_itf->bDescriptorType != TUSB_DESC_INTERFACE ||
                                                   (desc_itf->bInterfaceClass == TUSB_CLASS_AUDIO &&
                                                    desc_itf->bInterfaceSubClass != AUDIO_SUBCLASS_MIDI_STREAMING)))
    {
      if (desc_itf->bDescriptorType == TUSB_DESC_INTERFACE && desc_itf->bAlternateSetting == 0)
      {
        p_midi->itf_count++;
      }
      p_desc = tu_desc_next(p_desc);
      desc_itf = (const tusb_desc_interface_t *)p_desc;
    }
    TU_VERIFY(p_desc < desc_end, 0);
    TU_VERIFY(TUSB_CLASS_AUDIO == desc_itf->bInterfaceClass, 0);
  }
  TU_VERIFY(AUDIO_SUBCLASS_MIDI_STREAMING == desc_itf->bInterfaceSubClass, 0);

  TU_LOG_DRV("MIDI opening Interface %u (addr = %u)\r\n", desc_itf->bInterfaceNumber, dev_addr);
  p_midi->bInterfaceNumber = desc_itf->bInterfaceNumber;
  p_midi->iInterface = desc_itf->iInterface;
  p_midi->itf_count++;
  desc_cb.desc_midi = desc_itf;

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
        desc_cb.desc_header = p_desc;
        break;

      case MIDI_CS_INTERFACE_IN_JACK:
      case MIDI_CS_INTERFACE_OUT_JACK:
      {
        TU_LOG_DRV("  Jack %s %s descriptor \r\n",
                   tu_desc_subtype(p_desc) == MIDI_CS_INTERFACE_IN_JACK ? "IN" : "OUT",
                   p_desc[3] == MIDI_JACK_EXTERNAL ? "External" : "Embedded");
        if (desc_cb.jack_num < TU_ARRAY_SIZE(desc_cb.desc_jack))
        {
          desc_cb.desc_jack[desc_cb.jack_num++] = p_desc;
        }
        break;
      }

      case MIDI_CS_INTERFACE_ELEMENT:
        TU_LOG_DRV("  Element descriptor\r\n");
        desc_cb.desc_element = p_desc;
        break;

      case MIDI_CS_ROLAND_HEADER:
      {
        midi_desc_roland_header_t const *p_rl = (midi_desc_roland_header_t const *)p_desc;
        TU_LOG2("Found Roland Header %02x %02x %02x %02x\r\n", p_rl->bLength, p_rl->always_2, p_rl->num_cables_rx, p_rl->num_cables_tx);
        if (p_rl->bLength >= sizeof(midi_desc_roland_header_t) && p_rl->always_2 == 0x02)
        {
          TU_VERIFY(p_rl->num_cables_rx < 0x10 && p_rl->num_cables_tx < 0x10);
          TU_LOG2("Found Roland Header\r\n");
          p_midi->rx_cable_count = (1 << p_rl->num_cables_rx) - 1;
          p_midi->tx_cable_count = (1 << p_rl->num_cables_tx) - 1;
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
          desc_cb.desc_epout = p_ep;
          ep_stream = &usbMidi.ep_stream.tx;
        }
        else
        {
          desc_cb.desc_epin = p_ep;
          ep_stream = &usbMidi.ep_stream.rx;
        }
      }
      else
      {
        const midi_desc_cs_endpoint_t *p_csep = (const midi_desc_cs_endpoint_t *)p_desc;

        TU_LOG_DRV("  Endpoint and CS_Endpoint descriptor %02x\r\n", p_ep->bEndpointAddress);
        if (tu_edpt_dir(p_ep->bEndpointAddress) == TUSB_DIR_OUT)
        {
          p_midi->tx_cable_count = p_csep->bNumEmbMIDIJack;
          desc_cb.desc_epout = p_ep;
          ep_stream = &usbMidi.ep_stream.tx;
        }
        else
        {
          p_midi->rx_cable_count = p_csep->bNumEmbMIDIJack;
          desc_cb.desc_epin = p_ep;
          ep_stream = &usbMidi.ep_stream.rx;
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

  desc_cb.desc_midi_total_len = (uint16_t)((uintptr_t)p_desc - (uintptr_t)desc_start);

  p_midi->daddr = dev_addr;
  tuh_midi_descriptor_cb(idx, &desc_cb);

  return desc_cb.desc_midi_total_len;
}

bool midih_set_config(uint8_t dev_addr, uint8_t itf_num)
{
  uint8_t idx = tuh_midi_itf_get_index(dev_addr, itf_num);
  TU_ASSERT(idx < CFG_TUH_MIDI_CUSTOM);
  midih_interface_t *p_midi = &_midi_host[idx];
  p_midi->mounted = true;

  const tuh_midi_mount_cb_t mount_cb_data = {
      .daddr = dev_addr,
      .bInterfaceNumber = p_midi->bInterfaceNumber,
      .rx_cable_count = p_midi->rx_cable_count,
      .tx_cable_count = p_midi->tx_cable_count,
  };
  tuh_midi_mount_cb(idx, &mount_cb_data);

  tu_edpt_stream_read_xfer(&usbMidi.ep_stream.rx); // prepare for incoming data

  // No special config things to do for MIDI
  usbh_driver_set_config_complete(dev_addr, p_midi->bInterfaceNumber);
  return true;
}

//--------------------------------------------------------------------+
// API
//--------------------------------------------------------------------+
bool tuh_midi_mounted(uint8_t idx)
{
  TU_VERIFY(idx < CFG_TUH_MIDI_CUSTOM);
  midih_interface_t *p_midi = &_midi_host[idx];
  return p_midi->mounted;
}

uint8_t tuh_midi_itf_get_index(uint8_t daddr, uint8_t itf_num)
{
  for (uint8_t idx = 0; idx < CFG_TUH_MIDI_CUSTOM; idx++)
  {
    const midih_interface_t *p_midi = &_midi_host[idx];
    if (p_midi->daddr == daddr &&
        (p_midi->bInterfaceNumber == itf_num ||
         p_midi->bInterfaceNumber == (uint8_t)(itf_num + p_midi->itf_count - 1)))
    {
      return idx;
    }
  }
  return TUSB_INDEX_INVALID_8;
}

bool tuh_midi_itf_get_info(uint8_t idx, tuh_itf_info_t *info)
{
  midih_interface_t *p_midi = &_midi_host[idx];
  TU_VERIFY(p_midi && info);

  info->daddr = p_midi->daddr;

  // re-construct descriptor
  tusb_desc_interface_t *desc = &info->desc;
  desc->bLength = sizeof(tusb_desc_interface_t);
  desc->bDescriptorType = TUSB_DESC_INTERFACE;

  desc->bInterfaceNumber = p_midi->bInterfaceNumber;
  desc->bAlternateSetting = 0;
  desc->bNumEndpoints = 0;
  if (tu_edpt_stream_is_opened(&usbMidi.ep_stream.tx))
  {
    desc->bNumEndpoints++;
  }
  if (tu_edpt_stream_is_opened(&usbMidi.ep_stream.rx))
  {
    desc->bNumEndpoints++;
  }
  desc->bInterfaceClass = TUSB_CLASS_AUDIO;
  desc->bInterfaceSubClass = AUDIO_SUBCLASS_MIDI_STREAMING;
  desc->bInterfaceProtocol = 0;
  desc->iInterface = p_midi->iInterface;

  return true;
}

uint8_t tuh_midi_get_tx_cable_count(uint8_t idx)
{
  TU_VERIFY(idx < CFG_TUH_MIDI_CUSTOM);
  midih_interface_t *p_midi = &_midi_host[idx];
  TU_VERIFY(usbMidi.ep_stream.tx.ep_addr != 0, 0);
  return p_midi->tx_cable_count;
}

uint8_t tuh_midi_get_rx_cable_count(uint8_t idx)
{
  TU_VERIFY(idx < CFG_TUH_MIDI_CUSTOM);
  midih_interface_t *p_midi = &_midi_host[idx];
  TU_VERIFY(usbMidi.ep_stream.rx.ep_addr != 0, 0);
  return p_midi->rx_cable_count;
}

uint32_t tuh_midi_read_available(uint8_t idx)
{
  TU_VERIFY(idx < CFG_TUH_MIDI_CUSTOM);
  midih_interface_t *p_midi = &_midi_host[idx];
  return tu_edpt_stream_read_available(&usbMidi.ep_stream.rx);
}

uint32_t tuh_midi_write_flush(uint8_t idx)
{
  TU_VERIFY(idx < CFG_TUH_MIDI_CUSTOM);
  midih_interface_t *p_midi = &_midi_host[idx];
  return tu_edpt_stream_write_xfer(&usbMidi.ep_stream.tx);
}

//--------------------------------------------------------------------+
// Packet API
//--------------------------------------------------------------------+
uint32_t tuh_midi_packet_read_n(uint8_t idx, uint8_t *buffer, uint32_t bufsize)
{
  TU_VERIFY(idx < CFG_TUH_MIDI_CUSTOM && buffer && bufsize > 0, 0);
  midih_interface_t *p_midi = &_midi_host[idx];

  uint32_t count4 = tu_min32(bufsize, tu_edpt_stream_read_available(&usbMidi.ep_stream.rx));
  count4 = tu_align4(count4); // round down to multiple of 4
  TU_VERIFY(count4 > 0, 0);
  return tu_edpt_stream_read(&usbMidi.ep_stream.rx, buffer, count4);
}

uint32_t tuh_midi_packet_write_n(uint8_t idx, const uint8_t *buffer, uint32_t bufsize)
{
  TU_VERIFY(idx < CFG_TUH_MIDI_CUSTOM && buffer && bufsize > 0, 0);
  midih_interface_t *p_midi = &_midi_host[idx];

  const uint32_t bufsize4 = tu_align4(bufsize);
  TU_VERIFY(bufsize4 > 0, 0);
  return tu_edpt_stream_write(&usbMidi.ep_stream.tx, buffer, bufsize4);
}

#endif