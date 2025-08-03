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

#ifndef _TUSB_GH_ARCADE_DEVICE_H_
#define _TUSB_GH_ARCADE_DEVICE_H_

#include "common/tusb_common.h"
#include "device/usbd.h"

#ifndef CFG_TUD_GH_ARCADE_EPSIZE
#define CFG_TUD_GH_ARCADE_EPSIZE 64
#endif
#define GH_ARCADE_DESC_TYPE_RESERVED 0x21
#define GH_ARCADE_SECURITY_DESC_TYPE_RESERVED 0x41
#ifdef __cplusplus
extern "C"
{
#endif
    void gh_arcaded_init(void);
    void gh_arcaded_reset(uint8_t rhport);
    uint16_t gh_arcaded_open(uint8_t rhport, tusb_desc_interface_t const *itf_desc,
                             uint16_t max_len);
    bool gh_arcaded_control_request(uint8_t rhport,
                                    tusb_control_request_t const *request);
    bool gh_arcaded_control_complete(uint8_t rhport,
                                     tusb_control_request_t const *request);
    bool gh_arcaded_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t event,
                            uint32_t xferred_bytes);
    bool gh_arcaded_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request);
    void tud_gh_arcade_set_side_cb(uint8_t interface, uint8_t side);

#ifdef __cplusplus
}
#endif

#endif /* _TUSB_GH_ARCADE_DEVICE_H_ */
