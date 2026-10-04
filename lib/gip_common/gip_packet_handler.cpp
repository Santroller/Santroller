#include "gip_packet_handler.h"
#include "gip_device_mappings.h"
#include "gip_device.h"
#include "usb/auth_broker.h"
#include "managers/config_manager.hpp"
#include "../../lib/xgip_protocol/xgip_protocol.h"
#include "../../include/protocols/xbox_one.hpp"
#include <string.h>
#include <stdio.h>

// Shared default callback implementations

// While emulating (or about to emulate) an Xbox One controller, the console must
// authenticate the real controller, so never fake auth-complete locally.
static bool gip_console_auth_expected(void)
{
    // Host already declared auth complete, so complete late-connecting controllers locally.
    if (auth_broker.is_auth_completed(ModeXboxOne))
    {
        return false;
    }
    if (auth_broker.has_response_handler(ModeXboxOne))
    {
        return true;
    }
    auto &config_mgr = ConfigManager::instance();
    return config_mgr.get_current_mode() == ModeXboxOne ||
           config_mgr.get_requested_mode() == ModeXboxOne;
}

void gip_default_ack_callback(void *context)
{
    gip_device_t *device = (gip_device_t *)context;
    if (device)
    {
        device->waiting_ack = false;
    }
}

void gip_default_auth_callback(void *context, const uint8_t *data, uint16_t len)
{
    gip_device_t *device = (gip_device_t *)context;
    if (device && !device->auth_complete_sent)
    {
        gip_send_auth_complete(device);
    }
}

void gip_default_arrival_callback(void *context, void (*queue_packet)(void *, const uint8_t *, uint16_t))
{
    gip_device_t *device = (gip_device_t *)context;
    if (!device)
    {
        return;
    }

    gip_request_device_descriptor(device);
}

// Power-on sequence data
static const uint8_t XBOXONE_POWER_ON[] = {0x00};
static const uint8_t XBOXONE_POWER_ON_SINGLE[] = {0x00};
static const uint8_t XBOXONE_RUMBLE_STOP[] = {0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0xeb};
static const uint8_t XBOXONE_LED_ON[] = {0x00, 0x01, 0x14}; // 0x01 - LED on, 0x14 - Brightness

static uint16_t read_le16(const uint8_t *data)
{
    return data[0] | (data[1] << 8);
}

bool gip_process_packet(XGIPProtocol *xgip, gip_device_t *device)
{
    if (!xgip || !device || !device->interface)
    {
        return false;
    }

    const gip_device_interface_t *interface = device->interface;
    void *context = device->user_context;

    uint8_t command = xgip->getCommand();
    switch (command)
    {
    case GIP_ACK_RESPONSE:
        // Always use default ACK handler
        gip_default_ack_callback(device);
        return true;

    case GIP_DEVICE_DESCRIPTOR:
        device->console_function_offset = gip_parse_console_function_offset(xgip->getData(), xgip->getDataLength());
        if (interface->on_device_descriptor)
        {
            // Detect device subtype from descriptor data using shared mappings
            uint8_t subtype = gip_detect_device_subtype(
                xgip->getData(),
                xgip->getDataLength(),
                GIP_DEVICE_TYPE_MAPPINGS,
                GIP_DEVICE_TYPE_MAPPING_COUNT);

            // Pass detected subtype to callback
            interface->on_device_descriptor(context, (SubType)subtype);

            // Only hold off auth when an emulated Xbox One device (i.e. Xbox One
            // mode with a loaded profile) will pass the console's auth through.
            // Otherwise the controller sits unauthenticated and may fall back to
            // its wireless radio.
            if (!gip_console_auth_expected())
            {
                gip_default_auth_callback(
                    device,
                    xgip->getData(),
                    xgip->getDataLength());
            }
        }
        return true;

    case GIP_AUTH:
        // If an emulated (console-facing) device is passthrough-forwarding
        // auth for this mode, relay the real controller's auth response back
        // to it instead of faking an auth-complete here.
        if (auth_broker.has_response_handler(ModeXboxOne) && !auth_broker.is_auth_completed(ModeXboxOne))
        {
            auth_broker.forward_auth_response(ModeXboxOne, xgip);
        }
        else if (!gip_console_auth_expected())
        {
            // Not passing through auth, so use default auth handler (sends auth complete)
            gip_default_auth_callback(
                device,
                xgip->getData(),
                xgip->getDataLength());
        }
        return true;

    case GIP_INPUT_REPORT:
    case GIP_HID_REPORT:
        // Input data is already stored in device->raw_input by gip_device_process_incoming
        // No callback needed - consumers read from raw_input directly
        return true;

    case GIP_VIRTUAL_KEYCODE:
        // Handled in gip_device_process_incoming
        return true;

    case GIP_KEEPALIVE: // GIP_STATUS (0x03)
        if (xgip->getDataLength() >= 1)
        {
            uint8_t status = xgip->getData()[0];
            // Bit 7 (0x80) indicates connected state: GIP_STATUS_CONNECTED
            if (!(status & 0x80))
            {
                if (interface->on_disconnect)
                {
                    interface->on_disconnect(context);
                }
            }
        }
        return true;

    case GIP_ARRIVAL:
        if (interface->on_arrival)
        {
            interface->on_arrival(context);
        }
        return true;

    default:
        return false;
    }
}

uint8_t gip_detect_device_subtype(
    const uint8_t *data,
    uint16_t len,
    const gip_device_type_mapping_t *mappings,
    size_t mapping_count)
{
    if (!data || len < sizeof(BinaryMetadataHeader) + sizeof(BinaryDeviceMetadata))
    {
        return 0xFF;
    }

    // Skip BinaryMetadataHeader
    data += sizeof(BinaryMetadataHeader);
    len -= sizeof(BinaryMetadataHeader);

    BinaryDeviceMetadata *metadata = (BinaryDeviceMetadata *)data;

    // Check if we have enough data
    if (metadata->preferred_types_offset >= len)
    {
        return 0xFF;
    }

    // Move to preferred types
    data += metadata->preferred_types_offset;
    len -= metadata->preferred_types_offset;

    if (len < 1)
    {
        return 0xFF;
    }

    // First byte is count of preferred type strings
    uint8_t preferredTypeStrCount = *data++;
    len--;

    // Check each preferred type string
    for (size_t j = 0; j < preferredTypeStrCount; j++)
    {
        if (len < 2)
        {
            break;
        }

        uint16_t str_len = read_le16(data);
        data += 2;
        len -= 2;

        if (str_len > len)
        {
            break;
        }

        // Check against known device types
        for (size_t i = 0; i < mapping_count; i++)
        {
            if (strncmp((char *)data, mappings[i].name, str_len) == 0)
            {
                printf("GIP: Detected device type: %s (subtype %d)\r\n",
                       mappings[i].name, mappings[i].subtype);
                return mappings[i].subtype;
            }
        }

        // Move to next string
        data += str_len;
        len -= str_len;
    }

    return 0xFF; // Unknown device
}

// Windows.Xbox.Input.IConsoleFunctionMap {ECDDD2FE-D387-4294-BD96-1A712E3DC77D}, little-endian GUID layout
static const uint8_t GIP_CONSOLE_FUNCTION_MAP_GUID[16] = {
    0xFE, 0xD2, 0xDD, 0xEC, 0x87, 0xD3, 0x94, 0x42, 0xBD, 0x96, 0x1A, 0x71, 0x2E, 0x3D, 0xC7, 0x7D};

#define GIP_METADATA_MESSAGES_OFFSET 0   // BinaryDeviceMetadata::length is really the messages array offset
#define GIP_METADATA_INTERFACES_OFFSET 12 // BinaryDeviceMetadata::interfaces_offset
#define GIP_METADATA_MESSAGE_ENTRY_LENGTH 23

// Per MS-GIPUSB 3.1.5.6.1.3, input report extensions are appended to the end of the input report and
// the 0x20 message length in the metadata includes them. With IConsoleFunctionMap as the only extension,
// the map occupies the last 18 bytes of the report.
uint16_t gip_parse_console_function_offset(const uint8_t *data, uint16_t len)
{
    if (!data || len < sizeof(BinaryMetadataHeader) + sizeof(BinaryDeviceMetadata))
    {
        return 0;
    }
    const uint8_t *md = data + sizeof(BinaryMetadataHeader);
    uint16_t md_len = len - sizeof(BinaryMetadataHeader);

    uint16_t interfaces = read_le16(md + GIP_METADATA_INTERFACES_OFFSET);
    if (!interfaces || interfaces >= md_len)
    {
        return 0;
    }
    uint8_t count = md[interfaces];
    if (interfaces + 1 + count * 16 > md_len)
    {
        return 0;
    }
    bool supported = false;
    for (uint8_t i = 0; i < count && !supported; i++)
    {
        supported = memcmp(md + interfaces + 1 + i * 16, GIP_CONSOLE_FUNCTION_MAP_GUID, 16) == 0;
    }
    if (!supported)
    {
        return 0;
    }

    uint16_t messages = read_le16(md + GIP_METADATA_MESSAGES_OFFSET);
    if (!messages || messages >= md_len)
    {
        return 0;
    }
    count = md[messages];
    if (messages + 1 + count * GIP_METADATA_MESSAGE_ENTRY_LENGTH > md_len)
    {
        return 0;
    }
    for (uint8_t i = 0; i < count; i++)
    {
        const uint8_t *entry = md + messages + 1 + i * GIP_METADATA_MESSAGE_ENTRY_LENGTH;
        if (entry[2] != GIP_INPUT_REPORT)
        {
            continue;
        }
        uint16_t message_length = read_le16(entry + 3);
        if (message_length < GIP_CONSOLE_FUNCTION_MAP_LENGTH)
        {
            return 0;
        }
        return message_length - GIP_CONSOLE_FUNCTION_MAP_LENGTH;
    }
    return 0;
}

void gip_send_power_on_sequence(gip_device_t *device)
{
    if (!device || !device->outgoing_xgip || !device->interface || !device->interface->queue_packet)
    {
        return;
    }

    XGIPProtocol *xgip = device->outgoing_xgip;
    void *context = device->user_context;
    auto queue = device->interface->queue_packet;

    if (device->subtype == RockBandDrums || device->subtype == RockBandGuitar)
    {
        // Rock Band instruments (e.g. Mad Catz drums/strat) expect GIP_PWR_ON (mode 0x00),
        // LED on (to initialize Guide button LED ring), and auth complete.
        // Sending wired console MAC wireless pairing packets corrupts controller state.
        uint8_t seq1 = gip_sequence_pool_next(&device->tx_sequence_pools, GIP_POWER_MODE_DEVICE_CONFIG);
        xgip->reset();
        xgip->setAttributes(GIP_POWER_MODE_DEVICE_CONFIG, seq1, 1, 0, 0);
        xgip->setData(XBOXONE_POWER_ON_SINGLE, sizeof(XBOXONE_POWER_ON_SINGLE));
        queue(context, xgip->generatePacket(), xgip->getPacketLength());

        uint8_t seq2 = gip_sequence_pool_next(&device->tx_sequence_pools, GIP_CMD_LED_ON);
        xgip->reset();
        xgip->setAttributes(GIP_CMD_LED_ON, seq2, 1, 0, 0);
        xgip->setData(XBOXONE_LED_ON, sizeof(XBOXONE_LED_ON));
        queue(context, xgip->generatePacket(), xgip->getPacketLength());
        return;
    }

    if (device->subtype == LiveGuitar)
    {
        gip_send_ghl_poke(device);
        return;
    }

    // Standard gamepad power on sequence. Mirrors the generic xpad Xbox One
    // init packets that keep newer third-party pads in GIP mode.
    uint8_t seq1 = gip_sequence_pool_next(&device->tx_sequence_pools, GIP_POWER_MODE_DEVICE_CONFIG);
    xgip->reset();
    xgip->setAttributes(GIP_POWER_MODE_DEVICE_CONFIG, seq1, 1, 0, 0);
    xgip->setData(XBOXONE_POWER_ON, sizeof(XBOXONE_POWER_ON));
    queue(context, xgip->generatePacket(), xgip->getPacketLength());

    uint8_t seq2 = gip_sequence_pool_next(&device->tx_sequence_pools, GIP_CMD_LED_ON);
    xgip->reset();
    xgip->setAttributes(GIP_CMD_LED_ON, seq2, 1, 0, 0);
    xgip->setData(XBOXONE_LED_ON, sizeof(XBOXONE_LED_ON));
    queue(context, xgip->generatePacket(), xgip->getPacketLength());

    uint8_t seq3 = gip_sequence_pool_next(&device->tx_sequence_pools, GIP_CMD_RUMBLE);
    xgip->reset();
    xgip->setAttributes(GIP_CMD_RUMBLE, seq3, 0, 0, 0);
    xgip->setData(XBOXONE_RUMBLE_STOP, sizeof(XBOXONE_RUMBLE_STOP));
    queue(context, xgip->generatePacket(), xgip->getPacketLength());
}

void gip_request_device_descriptor(gip_device_t *device)
{
    if (!device || !device->outgoing_xgip || !device->interface || !device->interface->queue_packet)
    {
        return;
    }

    XGIPProtocol *xgip = device->outgoing_xgip;
    uint8_t seq = gip_sequence_pool_next(&device->tx_sequence_pools, GIP_DEVICE_DESCRIPTOR);

    xgip->reset();
    xgip->setAttributes(GIP_DEVICE_DESCRIPTOR, seq, 1, false, 0);
    device->interface->queue_packet(device->user_context, xgip->generatePacket(), xgip->getPacketLength());
}

void gip_send_auth_complete(gip_device_t *device)
{
    if (!device || device->auth_complete_sent || !device->outgoing_xgip || !device->interface || !device->interface->queue_packet)
    {
        return;
    }

    XGIPProtocol *xgip = device->outgoing_xgip;

    // Auth complete packet (0x06 command with 0x02 subcommand)
    static const uint8_t auth_complete[] = {0x01, 0x00};
    uint8_t seq = gip_sequence_pool_next(&device->tx_sequence_pools, GIP_AUTH);

    xgip->reset();
    xgip->setAttributes(GIP_AUTH, seq, 1, false, 0);
    xgip->setData(auth_complete, sizeof(auth_complete));
    device->interface->queue_packet(device->user_context, xgip->generatePacket(), xgip->getPacketLength());
    device->auth_complete_sent = true;

    // printf("GIP: Sent auth complete packet\n");
}

void gip_send_ghl_poke(gip_device_t *device)
{
    if (!device || !device->outgoing_xgip || !device->interface || !device->interface->queue_packet)
    {
        return;
    }

    static const uint8_t ghl_poke[] = {0x02, 0x08, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00};
    XGIPProtocol *xgip = device->outgoing_xgip;
    uint8_t seq = gip_sequence_pool_next(&device->tx_sequence_pools, GHL_HID_OUTPUT);

    xgip->reset();
    xgip->setAttributes(GHL_HID_OUTPUT, seq, 0, 0, 0);
    xgip->setData(ghl_poke, sizeof(ghl_poke));
    device->interface->queue_packet(device->user_context, xgip->generatePacket(), xgip->getPacketLength());
}
