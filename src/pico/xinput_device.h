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

#ifndef _TUSB_XINPUT_DEVICE_H_
#define _TUSB_XINPUT_DEVICE_H_

#include "common/tusb_common.h"
#include "descriptors.h"
#include "device/usbd.h"

#ifndef CFG_TUD_XINPUT_EPSIZE
#define CFG_TUD_XINPUT_EPSIZE 64
#endif
#define XINPUT_DESC_TYPE_RESERVED 0x21
#define XINPUT_SECURITY_DESC_TYPE_RESERVED 0x41
#ifdef __cplusplus
extern "C" {
#endif
// Check if the interface is ready to use
bool tud_xinput_n_ready(uint8_t itf);

// Check if current mode is Boot (true) or Report (false)
bool tud_xinput_n_boot_mode(uint8_t itf);

// Send report to host
bool tud_xinput_n_report(uint8_t itf, uint8_t report_id, void const *report,
                         uint8_t len);
bool tud_xusb_n_report(uint8_t itf, void const *report, uint8_t len);
void xinputd_init(void);
void xinputd_reset(uint8_t rhport);
uint16_t xinputd_open(uint8_t rhport, tusb_desc_interface_t const *itf_desc,
                      uint16_t max_len);
bool xinputd_control_request(uint8_t rhport,
                             tusb_control_request_t const *request);
bool xinputd_control_complete(uint8_t rhport,
                              tusb_control_request_t const *request);
bool xinputd_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t event,
                     uint32_t xferred_bytes);

#ifdef __cplusplus
}
#endif

#endif /* _TUSB_XINPUT_DEVICE_H_ */
