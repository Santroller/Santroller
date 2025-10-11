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
#include "device/usbd.h"
#include "gamepad_device.h"
#include "protocols/xinput.hpp"

#ifndef CFG_TUD_XINPUT_EPSIZE
#define CFG_TUD_XINPUT_EPSIZE 64
#endif
#define SERIAL_NUMBER_WVALUE 0x0000
#define INPUT_CAPABILITIES_WVALUE 0x0100
#define VIBRATION_CAPABILITIES_WVALUE 0x0000
#define XINPUT_DESC_TYPE_RESERVED 0x21
#define XINPUT_SECURITY_DESC_TYPE_RESERVED 0x41
#define XBOX_LED_ID 0x01
#define XBOX_RUMBLE_ID 0x00
typedef struct
{
    uint8_t bLength;         // Length of this descriptor.
    uint8_t bDescriptorType; // XINPUT_DESC_TYPE_RESERVED
    uint8_t reserved[2];
    uint8_t subtype;
    uint8_t reserved2;
    uint8_t bEndpointAddressIn;
    uint8_t bMaxDataSizeIn;
    uint8_t reserved3[5];
    uint8_t bEndpointAddressOut;
    uint8_t bMaxDataSizeOut;
    uint8_t reserved4[2];
} __attribute__((packed)) XBOX_ID_DESCRIPTOR;
typedef struct
{
    uint8_t bLength;         // Length of this descriptor.
    uint8_t bDescriptorType; // XINPUT_SECURITY_DESC_TYPE_RESERVED
    uint8_t reserved[4];
} __attribute__((packed)) XBOX_SECURITY_DESCRIPTOR;

enum
{
    XINPUT_GAMEPAD = 1,
    XINPUT_WHEEL = 2,
    XINPUT_ARCADE_STICK = 3,
    XINPUT_FLIGHT_STICK = 4,
    XINPUT_DANCE_PAD = 5,
    XINPUT_GUITAR = 6,
    XINPUT_GUITAR_ALTERNATE = 7,
    XINPUT_DRUMS = 8,
    XINPUT_STAGE_KIT = 9,
    XINPUT_GUITAR_BASS = 11,
    XINPUT_PRO_KEYS = 15,
    XINPUT_ARCADE_PAD = 19,
    XINPUT_TURNTABLE = 23,
    XINPUT_PRO_GUITAR = 25,
    XINPUT_DISNEY_INFINITY_AND_LEGO_DIMENSIONS = 33,
    XINPUT_SKYLANDERS = 36
};


#define XINPUT_FLAGS_NO_NAV 0xFFFF
#define XINPUT_FLAGS_NONE 0x0000
#define XINPUT_FLAGS_FORCE_FEEDBACK 0x000D
#ifdef __cplusplus
extern "C"
{
#endif
    // Check if the interface is ready to use
    bool tud_xinput_n_ready(uint8_t itf);

    // Send report to host
    bool tud_xinput_n_report(uint8_t itf, void const *report, uint8_t len);
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
    bool xinputd_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request);

#ifdef __cplusplus
}
#endif

#endif /* _TUSB_XINPUT_DEVICE_H_ */
