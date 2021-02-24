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

//--------------------------------------------------------------------+
// INCLUDE
//--------------------------------------------------------------------+
#  include "common/tusb_common.h"
#  include "device/usbd_pvt.h"
#  include "xinput_device.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF
//--------------------------------------------------------------------+
typedef struct {
  uint8_t itf_num;
  uint8_t ep_in;
  uint8_t ep_out;        // optional Out endpoint
  uint8_t boot_protocol; // Boot mouse or keyboard
  bool boot_mode;        // default = false (Report)
  uint8_t idle_rate;     // up to application to handle idle rate
  uint16_t report_desc_len;

  CFG_TUSB_MEM_ALIGN uint8_t epin_buf[CFG_TUD_HID_EP_BUFSIZE];
  CFG_TUSB_MEM_ALIGN uint8_t epout_buf[CFG_TUD_HID_EP_BUFSIZE];
} xinputd_interface_t;

CFG_TUSB_MEM_SECTION static xinputd_interface_t _xinputd_itf[CFG_TUD_HID];

/*------------- Helpers -------------*/
static inline uint8_t get_index_by_itfnum(uint8_t itf_num) {
  for (uint8_t i = 0; i < CFG_TUD_HID; i++) {
    if (itf_num == _xinputd_itf[i].itf_num) return i;
  }

  return 0xFF;
}

//--------------------------------------------------------------------+
// APPLICATION API
//--------------------------------------------------------------------+
bool tud_xinput_n_ready(uint8_t itf) {
  uint8_t const ep_in = _xinputd_itf[itf].ep_in;
  return tud_ready() && (ep_in != 0) && !usbd_edpt_busy(TUD_OPT_RHPORT, ep_in);
}

bool tud_xinput_n_report(uint8_t itf, uint8_t report_id, void const *report,
                         uint8_t len) {
  uint8_t const rhport = 0;
  xinputd_interface_t *p_xinput = &_xinputd_itf[itf];

  // claim endpoint
  TU_VERIFY(usbd_edpt_claim(rhport, p_xinput->ep_in));

  // prepare data
  if (report_id) {
    len = tu_min8(len, CFG_TUD_HID_EP_BUFSIZE - 1);

    p_xinput->epin_buf[0] = report_id;
    memcpy(p_xinput->epin_buf + 1, report, len);
    len++;
  } else {
    // If report id = 0, skip ID field
    len = tu_min8(len, CFG_TUD_HID_EP_BUFSIZE);
    memcpy(p_xinput->epin_buf, report, len);
  }

  return usbd_edpt_xfer(TUD_OPT_RHPORT, p_xinput->ep_in, p_xinput->epin_buf,
                        len);
}

bool tud_xinput_n_boot_mode(uint8_t itf) { return _xinputd_itf[itf].boot_mode; }

//--------------------------------------------------------------------+
// USBD-CLASS API
//--------------------------------------------------------------------+
void xinputd_init(void) { xinputd_reset(TUD_OPT_RHPORT); }

void xinputd_reset(uint8_t rhport) {
  (void)rhport;
  tu_memclr(_xinputd_itf, sizeof(_xinputd_itf));
}

uint16_t xinputd_open(uint8_t rhport, tusb_desc_interface_t const *itf_desc,
                      uint16_t max_len) {
  TU_VERIFY(TUSB_CLASS_VENDOR_SPECIFIC == itf_desc->bInterfaceClass, 0);
  uint16_t drv_len = sizeof(tusb_desc_interface_t) +
                     (itf_desc->bNumEndpoints * sizeof(tusb_desc_endpoint_t));

  TU_VERIFY(max_len >= drv_len, 0);
  if (itf_desc->bInterfaceSubClass != 0x5D ||
      itf_desc->bInterfaceProtocol != 0x01) {
    // We don't actually want to configure the config endpoint, so just skip
    // over it
    return drv_len;
  }

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

  return drv_len;
}

bool xinputd_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t result,
                     uint32_t xferred_bytes) {
  (void)result;

  uint8_t itf = 0;
  xinputd_interface_t *p_xinput = _xinputd_itf;

  for (;; itf++, p_xinput++) {
    if (itf >= TU_ARRAY_SIZE(_xinputd_itf)) return false;

    if (ep_addr == p_xinput->ep_out) break;
  }

  if (ep_addr == p_xinput->ep_out) {
    TU_ASSERT(usbd_edpt_xfer(rhport, p_xinput->ep_out, p_xinput->epout_buf,
                             sizeof(p_xinput->epout_buf)));
  }

  return true;
}

#endif
