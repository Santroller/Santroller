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
 */

#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_
#include "output/LUFAConfig.h"
#include "output/descriptors.h"

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------
// COMMON CONFIGURATION
//--------------------------------------------------------------------

// defined by compiler flags for flexibility
#ifndef CFG_TUSB_MCU
#  error CFG_TUSB_MCU must be defined
#endif

#if CFG_TUSB_MCU == OPT_MCU_LPC18XX || CFG_TUSB_MCU == OPT_MCU_LPC43XX ||      \
    CFG_TUSB_MCU == OPT_MCU_MIMXRT10XX || CFG_TUSB_MCU == OPT_MCU_NUC505 ||    \
    CFG_TUSB_MCU == OPT_MCU_CXD56
#  define CFG_TUSB_RHPORT0_MODE (OPT_MODE_DEVICE | OPT_MODE_HIGH_SPEED)
#else
#  define CFG_TUSB_RHPORT0_MODE OPT_MODE_DEVICE
#endif

#ifndef CFG_TUSB_OS
#  define CFG_TUSB_OS OPT_OS_PICO
#endif

// CFG_TUSB_DEBUG is defined by compiler in DEBUG build
// #define CFG_TUSB_DEBUG           0

/* USB DMA on some MCUs can only access a specific SRAM region with restriction
 * on alignment. Tinyusb use follows macros to declare transferring memory so
 * that they can be put into those specific section. e.g
 * - CFG_TUSB_MEM SECTION : __attribute__ (( section(".usb_ram") ))
 * - CFG_TUSB_MEM_ALIGN   : __attribute__ ((aligned(4)))
 */
#ifndef CFG_TUSB_MEM_SECTION
#  define CFG_TUSB_MEM_SECTION
#endif

#ifndef CFG_TUSB_MEM_ALIGN
#  define CFG_TUSB_MEM_ALIGN __attribute__((aligned(4)))
#endif

//--------------------------------------------------------------------
// DEVICE CONFIGURATION
//--------------------------------------------------------------------

#ifndef CFG_TUD_ENDPOINT0_SIZE
#  define CFG_TUD_ENDPOINT0_SIZE FIXED_CONTROL_ENDPOINT_SIZE
#endif

//------------- CLASS -------------//
#define CFG_TUD_HID 1
#define CFG_TUD_CDC 0
#define CFG_TUD_MSC 0
#define CFG_TUD_MIDI 1
#define CFG_TUD_VENDOR 0
#define CFG_TUD_XINPUT 2

// HID buffer size Should be sufficient to hold ID (if any) + Data
#define CFG_TUD_HID_EP_BUFSIZE HID_EPSIZE
#define CFG_TUD_MIDI_TX_BUFSIZE VENDOR_EPSIZE
#define CFG_TUD_MIDI_RX_BUFSIZE VENDOR_EPSIZE
#define CFG_TUD_VENDOR_EP_BUFSIZE VENDOR_EPSIZE
#define CFG_TUD_VENDOR_TX_BUFSIZE VENDOR_EPSIZE
#define CFG_TUD_VENDOR_RX_BUFSIZE VENDOR_EPSIZE
#define CFG_TUD_XINPUT_TX_BUFSIZE VENDOR_EPSIZE
#define CFG_TUD_XINPUT_RX_BUFSIZE VENDOR_EPSIZE


#ifdef __cplusplus
}
#endif

#endif /* _TUSB_CONFIG_H_ */