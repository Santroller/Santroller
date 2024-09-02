#pragma once

#define CFG_TUSB_MCU OPT_MCU_RP2040
#define CFG_TUSB_OS OPT_OS_PICO
#define CFG_TUSB_RHPORT0_MODE (OPT_MODE_DEVICE)
#define CFG_TUSB_RHPORT1_MODE (OPT_MODE_HOST)
// Allow usb hubs
#define CFG_TUH_DEVICE_MAX 8
// RHPort max operational speed can defined by board.mk
#ifndef BOARD_TUH_MAX_SPEED
#define BOARD_TUH_MAX_SPEED OPT_MODE_DEFAULT_SPEED
#endif
#define CFG_TUH_RPI_PIO_USB 1
#define CFG_TUH_HUB 1
#define CFG_TUD_CDC 0
#define CFG_TUD_MSC 0
#define CFG_TUD_MIDI 0
#define CFG_TUD_VENDOR 0
#define CFG_TUD_XINPUT 8
#define CFG_TUH_XINPUT 8
// #define CFG_TUSB_DEBUG 2
#define CFG_TUH_MIDI 1

// HID buffer size Should be sufficient to hold ID (if any) + Data
#define CFG_TUD_VENDOR_EP_BUFSIZE VENDOR_EPSIZE
#define CFG_TUD_VENDOR_TX_BUFSIZE VENDOR_EPSIZE
#define CFG_TUD_VENDOR_RX_BUFSIZE VENDOR_EPSIZE
#define CFG_TUD_XINPUT_TX_BUFSIZE 64
#define CFG_TUD_XINPUT_RX_BUFSIZE 64
#define CFG_TUD_HID_EP_IN_BUFSIZE 0x20
#define CFG_TUD_HID_EP_OUT_BUFSIZE 0x8
#define CFG_TUD_MIDI_TX_BUFSIZE 0x20
#define CFG_TUD_MIDI_RX_BUFSIZE 0x08
#ifndef CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_SECTION
#endif

#ifndef CFG_TUSB_MEM_ALIGN
#define CFG_TUSB_MEM_ALIGN          __attribute__ ((aligned(4)))
#endif
