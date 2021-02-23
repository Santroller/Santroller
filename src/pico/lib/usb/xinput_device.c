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

#if (TUSB_OPT_DEVICE_ENABLED && CFG_TUD_XINPUT)

#  include "device/usbd_pvt.h"
#  include "xinput_device.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF
//--------------------------------------------------------------------+
typedef struct {
  uint8_t itf_num;
  uint8_t ep_in;
  uint8_t ep_out;

  /*------------- From this point, data is not cleared by bus reset
   * -------------*/
  tu_fifo_t rx_ff;
  tu_fifo_t tx_ff;

  uint8_t rx_ff_buf[CFG_TUD_XINPUT_RX_BUFSIZE];
  uint8_t tx_ff_buf[CFG_TUD_XINPUT_TX_BUFSIZE];

#  if CFG_FIFO_MUTEX
  osal_mutex_def_t rx_ff_mutex;
  osal_mutex_def_t tx_ff_mutex;
#  endif

  // Endpoint Transfer buffer
  CFG_TUSB_MEM_ALIGN uint8_t epout_buf[CFG_TUD_XINPUT_EPSIZE];
  CFG_TUSB_MEM_ALIGN uint8_t epin_buf[CFG_TUD_XINPUT_EPSIZE];
} xinputd_interface_t;

CFG_TUSB_MEM_SECTION static xinputd_interface_t _xinputd_itf[CFG_TUD_XINPUT];

#  define ITF_MEM_RESET_SIZE offsetof(xinputd_interface_t, rx_ff)

bool tud_xinput_n_mounted(uint8_t itf) {
  return _xinputd_itf[itf].ep_in && _xinputd_itf[itf].ep_out;
}

uint32_t tud_xinput_n_available(uint8_t itf) {
  return tu_fifo_count(&_xinputd_itf[itf].rx_ff);
}

bool tud_xinput_n_peek(uint8_t itf, int pos, uint8_t *u8) {
  return tu_fifo_peek_at(&_xinputd_itf[itf].rx_ff, pos, u8);
}

//--------------------------------------------------------------------+
// Read API
//--------------------------------------------------------------------+
static void _prep_out_transaction(xinputd_interface_t *p_itf) {
  // skip if previous transfer not complete
  if (usbd_edpt_busy(TUD_OPT_RHPORT, p_itf->ep_out)) return;

  // Prepare for incoming data but only allow what we can store in the ring
  // buffer.
  uint16_t max_read = tu_fifo_remaining(&p_itf->rx_ff);
  if (max_read >= CFG_TUD_XINPUT_EPSIZE) {
    usbd_edpt_xfer(TUD_OPT_RHPORT, p_itf->ep_out, p_itf->epout_buf,
                   CFG_TUD_XINPUT_EPSIZE);
  }
}

uint32_t tud_xinput_n_read(uint8_t itf, void *buffer, uint32_t bufsize) {
  xinputd_interface_t *p_itf = &_xinputd_itf[itf];
  uint32_t num_read = tu_fifo_read_n(&p_itf->rx_ff, buffer, bufsize);
  _prep_out_transaction(p_itf);
  return num_read;
}

//--------------------------------------------------------------------+
// Write API
//--------------------------------------------------------------------+
static bool maybe_transmit(xinputd_interface_t *p_itf) {
  // skip if previous transfer not complete
  TU_VERIFY(!usbd_edpt_busy(TUD_OPT_RHPORT, p_itf->ep_in));

  uint16_t count =
      tu_fifo_read_n(&p_itf->tx_ff, p_itf->epin_buf, CFG_TUD_XINPUT_EPSIZE);
  if (count > 0) {
    TU_ASSERT(
        usbd_edpt_xfer(TUD_OPT_RHPORT, p_itf->ep_in, p_itf->epin_buf, count));
  }
  return true;
}

uint32_t tud_xinput_n_write(uint8_t itf, void const *buffer, uint32_t bufsize) {
  xinputd_interface_t *p_itf = &_xinputd_itf[itf];
  uint16_t ret = tu_fifo_write_n(&p_itf->tx_ff, buffer, bufsize);
  maybe_transmit(p_itf);
  return ret;
}

uint32_t tud_xinput_n_write_available(uint8_t itf) {
  return tu_fifo_remaining(&_xinputd_itf[itf].tx_ff);
}

//--------------------------------------------------------------------+
// USBD Driver API
//--------------------------------------------------------------------+
void xinputd_init(void) {
  tu_memclr(_xinputd_itf, sizeof(_xinputd_itf));

  for (uint8_t i = 0; i < CFG_TUD_XINPUT; i++) {
    xinputd_interface_t *p_itf = &_xinputd_itf[i];

    // config fifo
    tu_fifo_config(&p_itf->rx_ff, p_itf->rx_ff_buf, CFG_TUD_XINPUT_RX_BUFSIZE,
                   1, false);
    tu_fifo_config(&p_itf->tx_ff, p_itf->tx_ff_buf, CFG_TUD_XINPUT_TX_BUFSIZE,
                   1, false);

#  if CFG_FIFO_MUTEX
    tu_fifo_config_mutex(&p_itf->rx_ff, osal_mutex_create(&p_itf->rx_ff_mutex));
    tu_fifo_config_mutex(&p_itf->tx_ff, osal_mutex_create(&p_itf->tx_ff_mutex));
#  endif
  }
}

void xinputd_reset(uint8_t rhport) {
  (void)rhport;

  for (uint8_t i = 0; i < CFG_TUD_XINPUT; i++) {
    xinputd_interface_t *p_itf = &_xinputd_itf[i];

    tu_memclr(p_itf, ITF_MEM_RESET_SIZE);
    tu_fifo_clear(&p_itf->rx_ff);
    tu_fifo_clear(&p_itf->tx_ff);
  }
}

uint16_t xinputd_open(uint8_t rhport, tusb_desc_interface_t const *itf_desc,
                      uint16_t max_len) {
  TU_VERIFY(TUSB_CLASS_VENDOR_SPECIFIC == itf_desc->bInterfaceClass, 0);
  uint16_t drv_len = sizeof(tusb_desc_interface_t) +
                     (itf_desc->bNumEndpoints * sizeof(tusb_desc_endpoint_t));
  TU_VERIFY(max_len >= drv_len, 0);

  // Find available interface
  xinputd_interface_t *p_xinput = NULL;
  for (uint8_t i = 0; i < CFG_TUD_XINPUT; i++) {
    if (_xinputd_itf[i].ep_in == 0 && _xinputd_itf[i].ep_out == 0) {
      p_xinput = &_xinputd_itf[i];
      break;
    }
  }
  TU_VERIFY(p_xinput, 0);
  uint8_t const *p_desc = (uint8_t const *)itf_desc;

  // Xinput reserved endpoint
  if (itf_desc->bInterfaceSubClass == 0x5D &&
      itf_desc->bInterfaceProtocol == 0x01) {
    //-------------- Xinput Descriptor --------------//
    p_desc = tu_desc_next(p_desc);
    USB_HID_XBOX_Descriptor_HID_t *x_desc =
        (USB_HID_XBOX_Descriptor_HID_t *)p_desc;
    TU_ASSERT(XINPUT_DESC_TYPE_RESERVED == x_desc->Header.Type, 0);
    drv_len += sizeof(USB_HID_XBOX_Descriptor_HID_t);

    //------------- Endpoint Descriptor -------------//
    p_desc = tu_desc_next(p_desc);
    TU_ASSERT(usbd_open_edpt_pair(rhport, p_desc, 2, TUSB_XFER_INTERRUPT,
                                  &p_xinput->ep_out, &p_xinput->ep_in),
              0);

    p_xinput->itf_num = itf_desc->bInterfaceNumber;

    // Prepare for incoming data
    if (!usbd_edpt_xfer(rhport, p_xinput->ep_out, p_xinput->epout_buf,
                        sizeof(p_xinput->epout_buf))) {
      TU_LOG1_FAILED();
      TU_BREAKPOINT();
    }
    // Config endpoint
  } else {
    p_desc = tu_desc_next(p_desc);
    TU_ASSERT(usbd_open_edpt_pair(rhport, p_desc, 1, TUSB_XFER_BULK,
                                  &p_xinput->ep_out, &p_xinput->ep_in),
              0);
    p_xinput->itf_num = itf_desc->bInterfaceNumber;
  }

  return drv_len;
}

bool xinputd_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t result,
                     uint32_t xferred_bytes) {
  (void)rhport;
  (void)result;

  uint8_t itf = 0;
  xinputd_interface_t *p_itf = _xinputd_itf;

  for (;; itf++, p_itf++) {
    if (itf >= TU_ARRAY_SIZE(_xinputd_itf)) return false;

    if ((ep_addr == p_itf->ep_out) || (ep_addr == p_itf->ep_in)) break;
  }

  if (ep_addr == p_itf->ep_out) {
    // Receive new data
    tu_fifo_write_n(&p_itf->rx_ff, p_itf->epout_buf, xferred_bytes);

    // Invoked callback if any
    if (tud_xinput_rx_cb) tud_xinput_rx_cb(itf);

    _prep_out_transaction(p_itf);
  } else if (ep_addr == p_itf->ep_in) {
    // Send complete, try to send more if possible
    maybe_transmit(p_itf);
  }

  return true;
}

#endif
