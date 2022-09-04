#pragma once

#define CFG_TUSB_RHPORT0_MODE (OPT_MODE_DEVICE)
#define CFG_TUSB_RHPORT1_MODE (OPT_MODE_HOST)
#ifndef BOARD_TUD_RHPORT
#define BOARD_TUD_RHPORT 0
#endif

// RHPort max operational speed can defined by board.mk
#ifndef BOARD_TUD_MAX_SPEED
#define BOARD_TUD_MAX_SPEED OPT_MODE_DEFAULT_SPEED
#endif

// RHPort number used for host can be defined by board.mk, default to port 1
#ifndef BOARD_TUH_RHPORT
#define BOARD_TUH_RHPORT 1
#endif

// RHPort max operational speed can defined by board.mk
#ifndef BOARD_TUH_MAX_SPEED
#define BOARD_TUH_MAX_SPEED OPT_MODE_DEFAULT_SPEED
#endif
#define CFG_TUH_RPI_PIO_USB 1
#define CFG_TUD_CDC 0
#define CFG_TUD_MSC 0
#define CFG_TUD_MIDI 0
#define CFG_TUD_VENDOR 0
#define CFG_TUD_XINPUT 4
#define CFG_TUH_XINPUT 4
// #define CFG_TUSB_DEBUG 1
#define CFG_TUSB_DEBUG 2

// HID buffer size Should be sufficient to hold ID (if any) + Data
#define CFG_TUD_HID_EP_BUFSIZE 0x20
#define CFG_TUD_MIDI_TX_BUFSIZE 0x20
#define CFG_TUD_MIDI_RX_BUFSIZE 0x08
#define CFG_TUD_VENDOR_EP_BUFSIZE VENDOR_EPSIZE
#define CFG_TUD_VENDOR_TX_BUFSIZE VENDOR_EPSIZE
#define CFG_TUD_VENDOR_RX_BUFSIZE VENDOR_EPSIZE
#define CFG_TUD_XINPUT_TX_BUFSIZE 0x18
#define CFG_TUD_XINPUT_RX_BUFSIZE 0x08
