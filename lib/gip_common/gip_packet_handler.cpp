#include "gip_packet_handler.h"
#include "gip_device_mappings.h"
#include "gip_device.h"
#include "usb/auth_broker.h"
#include "../../lib/xgip_protocol/xgip_protocol.h"
#include "../../include/protocols/xbox_one.hpp"
#include <string.h>
#include <stdio.h>

// Shared default callback implementations

void gip_default_ack_callback(void *context)
{
    gip_device_t *device = (gip_device_t *)context;
    if (device) {
        device->waiting_ack = false;
    }
}

void gip_default_auth_callback(void *context, const uint8_t *data, uint16_t len)
{
    gip_device_t *device = (gip_device_t *)context;
    if (device) {
        gip_send_auth_complete(device);
    }
}

void gip_default_arrival_callback(void *context, void (*queue_packet)(void *, const uint8_t *, uint16_t))
{
    gip_device_t *device = (gip_device_t *)context;
    if (!device) {
        return;
    }
    
    gip_request_device_descriptor(device);
}

// Power-on sequence data
static const uint8_t XBOXONE_POWER_ON[] = {0x06, 0x62, 0x45, 0xb8, 0x77, 0x26, 0x2c, 0x55,
                                           0x53, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f};
static const uint8_t XBOXONE_POWER_ON_SINGLE[] = {0x00};
static const uint8_t XBOXONE_RUMBLE_ON[] = {0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0xeb};
static const uint8_t XBOXONE_LED_ON[] = {0x00, 0x01, 0x14}; // 0x01 - LED on, 0x14 - Brightness

static uint16_t read_le16(const uint8_t *data)
{
    return data[0] | (data[1] << 8);
}

bool gip_process_packet(XGIPProtocol *xgip, gip_device_t *device)
{
    if (!xgip || !device || !device->interface) {
        return false;
    }
    
    const gip_device_interface_t *interface = device->interface;
    void *context = device->user_context;
    
    uint8_t command = xgip->getCommand();
    switch (command) {
        case GIP_ACK_RESPONSE:
            // Always use default ACK handler
            gip_default_ack_callback(device);
            return true;
            
        case GIP_DEVICE_DESCRIPTOR:
            if (interface->on_device_descriptor) {
                // Detect device subtype from descriptor data using shared mappings
                uint8_t subtype = gip_detect_device_subtype(
                    xgip->getData(),
                    xgip->getDataLength(),
                    GIP_DEVICE_TYPE_MAPPINGS,
                    GIP_DEVICE_TYPE_MAPPING_COUNT
                );
                
                // Pass detected subtype to callback
                interface->on_device_descriptor(context, (SubType)subtype);
                
                // not emulating xb1, so skip auth
                if (!auth_broker.get_auth_device(ModeXboxOne)) {
                    gip_default_auth_callback(
                    device,
                    xgip->getData(),
                    xgip->getDataLength()
                );
                }
            }
            return true;
            
        case GIP_AUTH:
            // Always use default auth handler (sends auth complete)
            gip_default_auth_callback(
                device,
                xgip->getData(),
                xgip->getDataLength()
            );
            return true;
            
        case GIP_INPUT_REPORT:
            // Input data is already stored in device->raw_input by gip_device_process_incoming
            // No callback needed - consumers read from raw_input directly
            return true;
            
        case GIP_ARRIVAL:
            if (interface->on_arrival) {
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
    if (!data || len < sizeof(BinaryMetadataHeader) + sizeof(BinaryDeviceMetadata)) {
        return 0xFF;
    }
    
    // Skip BinaryMetadataHeader
    data += sizeof(BinaryMetadataHeader);
    len -= sizeof(BinaryMetadataHeader);
    
    BinaryDeviceMetadata *metadata = (BinaryDeviceMetadata *)data;
    
    // Check if we have enough data
    if (metadata->preferred_types_offset >= len) {
        return 0xFF;
    }
    
    // Move to preferred types
    data += metadata->preferred_types_offset;
    len -= metadata->preferred_types_offset;
    
    if (len < 1) {
        return 0xFF;
    }
    
    // First byte is count of preferred type strings
    uint8_t preferredTypeStrCount = *data++;
    len--;
    
    // Check each preferred type string
    for (size_t j = 0; j < preferredTypeStrCount; j++) {
        if (len < 2) {
            break;
        }
        
        uint16_t str_len = read_le16(data);
        data += 2;
        len -= 2;
        
        if (str_len > len) {
            break;
        }
        
        // Check against known device types
        for (size_t i = 0; i < mapping_count; i++) {
            if (strncmp((char *)data, mappings[i].name, str_len) == 0) {
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

void gip_send_power_on_sequence(gip_device_t *device)
{
    if (!device || !device->outgoing_xgip || !device->interface || !device->interface->queue_packet) {
        return;
    }
    
    XGIPProtocol *xgip = device->outgoing_xgip;
    void *context = device->user_context;
    auto queue = device->interface->queue_packet;
    
    // Power on command 1
    xgip->reset();
    xgip->setAttributes(GIP_POWER_MODE_DEVICE_CONFIG, 2, 1, 0, 0);
    xgip->setData(XBOXONE_POWER_ON, sizeof(XBOXONE_POWER_ON));
    queue(context, xgip->generatePacket(), xgip->getPacketLength());
    
    // Power on command 2
    xgip->reset();
    xgip->setAttributes(GIP_POWER_MODE_DEVICE_CONFIG, 3, 1, 0, 0);
    xgip->setData(XBOXONE_POWER_ON_SINGLE, sizeof(XBOXONE_POWER_ON_SINGLE));
    queue(context, xgip->generatePacket(), xgip->getPacketLength());
    
    // LED on
    xgip->reset();
    xgip->setAttributes(GIP_CMD_LED_ON, 1, 1, 0, 0);
    xgip->setData(XBOXONE_LED_ON, sizeof(XBOXONE_LED_ON));
    queue(context, xgip->generatePacket(), xgip->getPacketLength());
    
    // Rumble on
    xgip->reset();
    xgip->setAttributes(GIP_POWER_MODE_DEVICE_CONFIG, 1, 1, 0, 0);
    xgip->setData(XBOXONE_RUMBLE_ON, sizeof(XBOXONE_RUMBLE_ON));
    queue(context, xgip->generatePacket(), xgip->getPacketLength());
}

void gip_request_device_descriptor(gip_device_t *device)
{
    if (!device || !device->outgoing_xgip || !device->interface || !device->interface->queue_packet) {
        return;
    }
    
    XGIPProtocol *xgip = device->outgoing_xgip;
    
    xgip->reset();
    xgip->setAttributes(GIP_DEVICE_DESCRIPTOR, 1, 1, false, 0);
    device->interface->queue_packet(device->user_context, xgip->generatePacket(), xgip->getPacketLength());
}

void gip_send_auth_complete(gip_device_t *device)
{
    if (!device || !device->outgoing_xgip || !device->interface || !device->interface->queue_packet) {
        return;
    }
    
    XGIPProtocol *xgip = device->outgoing_xgip;
    
    // Auth complete packet (0x06 command with 0x02 subcommand)
    static const uint8_t auth_complete[] = {0x02, 0x00};
    
    xgip->reset();
    xgip->setAttributes(GIP_AUTH, 1, 1, false, 0);
    xgip->setData(auth_complete, sizeof(auth_complete));
    device->interface->queue_packet(device->user_context, xgip->generatePacket(), xgip->getPacketLength());
    
    printf("GIP: Sent auth complete packet\n");
}
