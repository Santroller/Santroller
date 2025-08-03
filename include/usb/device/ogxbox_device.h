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

#ifndef _TUSB_OGXBOX_DEVICE_H_
#define _TUSB_OGXBOX_DEVICE_H_

#include "common/tusb_common.h"
#include "device/usbd.h"

#ifndef CFG_TUD_OGXBOX_EPSIZE
#define CFG_TUD_OGXBOX_EPSIZE 64
#endif

typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bcdXid;
    uint8_t bType;
    uint8_t bSubType;
    uint8_t bMaxInputReportSize;
    uint8_t bMaxOutputReportSize;
    uint16_t wAlternateProductIds[4];
} __attribute__((packed)) XID_DESCRIPTOR;


typedef struct {
    uint8_t rid;
    uint8_t rsize;
    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;
    uint8_t start : 1;
    uint8_t back : 1;  // select
    uint8_t leftThumbClick : 1;   // l3
    uint8_t rightThumbClick : 1;  // r3
    uint8_t padding;
    uint8_t a;
    uint8_t b;
    uint8_t x;
    uint8_t y;
    uint8_t leftShoulder;
    uint8_t rightShoulder;
    uint8_t leftTrigger;
    uint8_t rightTrigger;
    uint16_t leftStickX;
    uint16_t leftStickY;
    uint16_t rightStickX;
    uint16_t rightStickY;
} __attribute__((packed)) OGXboxGamepadCapabilities_Data_t;


typedef struct {
    uint8_t rid;
    uint8_t rsize;
    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;
    uint8_t start : 1;
    uint8_t back : 1;  // select
    uint8_t leftThumbClick : 1;   // l3
    uint8_t rightThumbClick : 1;  // r3
    uint8_t padding;
    uint8_t a;
    uint8_t b;
    uint8_t x;
    uint8_t y;
    uint8_t rightShoulder; // black
    uint8_t leftShoulder; // white
    uint8_t leftTrigger;
    uint8_t rightTrigger;
    int16_t leftStickX;
    int16_t leftStickY;
    int16_t rightStickX;
    int16_t rightStickY;
} __attribute__((packed)) OGXboxGamepad_Data_t;


typedef struct {
    uint8_t  rid; 
    uint8_t  rsize;
    uint16_t left;
    uint16_t right;
} __attribute__((packed)) OGXboxOutput_Report_t;

#ifdef __cplusplus
extern "C"
{
#endif
    // Check if the interface is ready to use
    bool tud_ogxbox_n_ready(uint8_t itf);

    // Send report to host
    bool tud_ogxbox_n_report(uint8_t itf, void const *report, uint8_t len);
    void ogxboxd_init(void);
    void ogxboxd_reset(uint8_t rhport);
    uint16_t ogxboxd_open(uint8_t rhport, tusb_desc_interface_t const *itf_desc,
                          uint16_t max_len);
    bool ogxboxd_control_request(uint8_t rhport,
                                 tusb_control_request_t const *request);
    bool ogxboxd_control_complete(uint8_t rhport,
                                  tusb_control_request_t const *request);
    bool ogxboxd_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t event,
                         uint32_t xferred_bytes);
    bool ogxboxd_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request);
    void tud_ogxbox_set_report_cb(uint8_t interface, uint8_t const *buf, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* _TUSB_OGXBOX_DEVICE_H_ */
