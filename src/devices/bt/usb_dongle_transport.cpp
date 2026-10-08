#include "devices/bt/usb_dongle_transport.hpp"
#include "devices/bt/usb_dongle.hpp"
#include "devices/bt/bluetooth_stack.hpp"
#include "btstack.h"
#include <stdio.h>

// BTstack HCI transport over a USB bluetooth adapter (H2), see usb_dongle.hpp. The stack can be started
// before an adapter is plugged in: HCI just keeps trying to reset the controller until one turns up.

static void (*s_packet_handler)(uint8_t packet_type, uint8_t *packet, uint16_t size) = nullptr;
static bool s_open = false;
static bool s_restart = false;

// set to 1 to log HCI commands and events sent to and from the adapter
#define BT_DONGLE_DEBUG 0

static void emit_event(const uint8_t *event, uint16_t len)
{
    if (s_open && s_packet_handler)
    {
        s_packet_handler(HCI_EVENT_PACKET, const_cast<uint8_t *>(event), len);
    }
}

void usb_dongle_attached()
{
    BtStackLock lock;
    printf("BT dongle: bluetooth adapter connected\r\n");
    // HCI may have been waiting to send the reset, let it know it can now
    static const uint8_t packet_sent[] = {HCI_EVENT_TRANSPORT_PACKET_SENT, 0};
    emit_event(packet_sent, sizeof(packet_sent));
}

void usb_dongle_detached()
{
    BtStackLock lock;
    printf("BT dongle: bluetooth adapter disconnected\r\n");
    // restart HCI, so it drops every connection and then waits for the next adapter
    s_restart = true;
}

void usb_dongle_hardware_error(uint8_t error)
{
    printf("BT dongle: adapter reported a hardware error 0x%02x, restarting bluetooth\r\n", error);
    s_restart = true;
}

bool usb_dongle_take_restart()
{
    bool restart = s_restart;
    s_restart = false;
    return restart;
}

void usb_dongle_packet_received(uint8_t packet_type, uint8_t *packet, uint16_t len)
{
    BtStackLock lock;
#if BT_DONGLE_DEBUG
    if (packet_type == USB_DONGLE_EVENT_PACKET)
    {
        printf("BT dongle: event 0x%02x len %u %02x %02x %02x %02x\r\n", packet[0], len, packet[2], packet[3], packet[4], packet[5]);
    }
#endif
    if (s_open && s_packet_handler)
    {
        s_packet_handler(packet_type, packet, len);
    }
}

void usb_dongle_packet_sent()
{
    BtStackLock lock;
    static const uint8_t packet_sent[] = {HCI_EVENT_TRANSPORT_PACKET_SENT, 0};
    emit_event(packet_sent, sizeof(packet_sent));
}

static void transport_init(const void *transport_config)
{
    UNUSED(transport_config);
}

static int transport_open(void)
{
    s_open = true;
    return 0;
}

static int transport_close(void)
{
    s_open = false;
    return 0;
}

static void transport_register_packet_handler(void (*handler)(uint8_t packet_type, uint8_t *packet, uint16_t size))
{
    s_packet_handler = handler;
}

static int transport_can_send_packet_now(uint8_t packet_type)
{
    return usb_dongle_can_send(packet_type);
}

static int transport_send_packet(uint8_t packet_type, uint8_t *packet, int size)
{
    bool sent = usb_dongle_send(packet_type, packet, size);
#if BT_DONGLE_DEBUG
    if (packet_type == USB_DONGLE_COMMAND_PACKET)
    {
        printf("BT dongle: command 0x%04x %s\r\n", little_endian_read_16(packet, 0), sent ? "sent" : "FAILED");
    }
#endif
    return sent ? 0 : -1;
}

const hci_transport_t *usb_dongle_transport_instance()
{
    static const hci_transport_t instance = {
        /* name */ "H2_USB_DONGLE",
        /* init */ &transport_init,
        /* open */ &transport_open,
        /* close */ &transport_close,
        /* register_packet_handler */ &transport_register_packet_handler,
        /* can_send_packet_now */ &transport_can_send_packet_now,
        /* send_packet */ &transport_send_packet,
        /* set_baudrate */ nullptr,
        /* reset_link */ nullptr,
        /* set_sco_config */ nullptr,
    };
    return &instance;
}
