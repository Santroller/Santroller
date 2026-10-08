#pragma once
#include <stdint.h>

// Bridge between the USB host driver for a bluetooth adapter (bt_dongle_host.cpp, TinyUSB side) and the
// BTstack HCI transport that runs over it (usb_dongle_transport.cpp, BTstack side). It is split like
// this, and only uses plain types, as the TinyUSB and BTstack headers can't be included together.
// Everything here runs from the main loop.

// HCI packet types, as BTstack numbers them
#define USB_DONGLE_COMMAND_PACKET 0x01
#define USB_DONGLE_ACL_PACKET 0x02
#define USB_DONGLE_EVENT_PACKET 0x04

// Largest packets we accept from the adapter: an event is at most 2 + 255 bytes, and ACL packets
// are at most the 4 byte header plus HCI_ACL_PAYLOAD_SIZE (see btstack_config.h)
#define USB_DONGLE_EVENT_BUFFER_SIZE 260
#define USB_DONGLE_ACL_BUFFER_SIZE (4 + 1691 + 4)

// Implemented by the USB host driver
bool usb_dongle_present();
bool usb_dongle_can_send(uint8_t packet_type);
bool usb_dongle_send(uint8_t packet_type, const uint8_t *packet, uint16_t len);

// Implemented by the HCI transport
void usb_dongle_attached();
void usb_dongle_detached();
void usb_dongle_packet_received(uint8_t packet_type, uint8_t *packet, uint16_t len);
void usb_dongle_packet_sent();
