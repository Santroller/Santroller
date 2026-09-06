#include "gip_device.h"
#include "gip_packet_handler.h"
#include "gip_report_queue.h"
#include "../../lib/xgip_protocol/xgip_protocol.h"
#include "../../include/protocols/xbox_one.hpp"
#include <string.h>
#include <stdio.h>

// MS-GIPUSB "Reliable Message Acknowledgement" (5a4be035-9e20-4350-b81a-a08627a985ca)
#define GIP_RELIABLE_MESSAGE_TIMEOUT_MS 1000
#define GIP_RELIABLE_MESSAGE_ACK_INTERVAL_MS 100

// Fires on every parsed packet; printf blocks on real UART hardware drain (confirmed via gdb),
// so keep this off outside active tracing sessions.
#define GIP_TRACE_ENABLED 0
#define GIP_RELIABLE_MESSAGE_MAX_HEARTBEAT_ACKS 8

void gip_device_init(gip_device_t *device)
{
    if (!device) return;
    
    memset(device, 0, sizeof(*device));
    gip_sequence_pool_init(&device->tx_sequence_pools);
    device->incoming_xgip = new XGIPProtocol();
    device->outgoing_xgip = new XGIPProtocol();
}

void gip_device_cleanup(gip_device_t *device)
{
    if (!device) return;
    
    if (device->incoming_xgip) {
        delete device->incoming_xgip;
        device->incoming_xgip = nullptr;
    }
    
    if (device->outgoing_xgip) {
        delete device->outgoing_xgip;
        device->outgoing_xgip = nullptr;
    }
}

bool gip_device_process_incoming(
    gip_device_t *device,
    const uint8_t *data,
    uint16_t len)
{
    if (!device || !device->incoming_xgip || !device->interface) {
        return false;
    }

    bool processed_any = false;

    while (len >= 4) {
        // Trailing padding bytes in Wi-Fi frame or USB transfer: 0x00 is never a valid GIP command
        if (data[0] == 0) {
            break;
        }

        // Feed data to incoming XGIP protocol parser
        if (!device->incoming_xgip->parse(data, len)) {
#if GIP_TRACE_ENABLED
            printf("gip_device_process_incoming: PARSE FAILED, cmd=0x%02X, len=%d. Hex:", data[0], len);
            for (int i = 0; i < len && i < 32; i++) {
                printf(" %02X", data[i]);
            }
            printf("\n");
#endif
            break;
        }

        uint16_t wire_len = device->incoming_xgip->getParsedWireLength();
        if (wire_len == 0 || wire_len > len) {
#if GIP_TRACE_ENABLED
            printf("gip_device_process_incoming: INVALID wire_len=%d, remaining len=%d\n", wire_len, len);
#endif
            break;
        }

#if GIP_TRACE_ENABLED
        if (device->incoming_xgip->getCommand() != GIP_INPUT_REPORT) {
            printf("gip_device_process_incoming: parsed cmd=0x%02X, wire_len=%d, chunked=%d, endOfChunk=%d, ackRequired=%d\n",
                   device->incoming_xgip->getCommand(), wire_len, device->incoming_xgip->getChunked(),
                   device->incoming_xgip->endOfChunk(), device->incoming_xgip->ackRequired());
        }
#endif

        // Immediately dispatch ACK if requested by incoming packet
        if (device->incoming_xgip->ackRequired()) {
            uint8_t *ack_data = device->incoming_xgip->generateAckPacket();
            uint16_t ack_len = device->incoming_xgip->getPacketLength();
            if (device->interface->send_ack) {
                device->interface->send_ack(device->user_context, ack_data, ack_len);
            } else if (device->interface->queue_packet) {
                device->interface->queue_packet(device->user_context, ack_data, ack_len);
            }
        }

        if (device->incoming_xgip->getChunked()) {
            if (!device->incoming_xgip->endOfChunk()) {
                // Mid-transfer chunk fragment; accumulate and wait for the terminating chunk.
                // Timestamps are (re)established on the next gip_device_update() tick.
                device->incoming_chunk_pending = true;
                device->incoming_chunk_last_data_at = 0;
                device->incoming_chunk_heartbeat_acks = 0;
                data += wire_len;
                len -= wire_len;
                processed_any = true;
                continue;
            }
            device->incoming_chunk_pending = false;
            device->incoming_chunk_last_data_at = 0;
            device->incoming_chunk_last_ack_at = 0;
            device->incoming_chunk_heartbeat_acks = 0;
        }

        // Store input report data directly in raw_input buffer
        if (device->incoming_xgip->getCommand() == GIP_INPUT_REPORT) {
            const uint8_t *input_data = device->incoming_xgip->getData();
            uint16_t input_len = device->incoming_xgip->getDataLength();
            if (input_len <= sizeof(device->raw_input)) {
                memcpy(device->raw_input, input_data, input_len);
                if (device->has_virtual_key_guide) {
                    auto gamepad = (XboxOneGamepad_Data_t *)device->raw_input;
                    gamepad->guide = gamepad->guide | (device->virtual_key_guide ? 1 : 0);
                }
            }
#if GIP_TRACE_ENABLED
            static uint16_t last_buttons = 0xFFFF;
            uint16_t cur_btn = (input_len >= 2) ? (input_data[0] | (input_data[1] << 8)) : 0;
            if (cur_btn != last_buttons) {
                last_buttons = cur_btn;
                printf("GIP Input 0x20 [len=%d]: (btn=0x%04X, guide_bit=%d)\n", input_len, cur_btn, (input_data[0] >> 1) & 1);
            }
#endif
        }
        else if (device->incoming_xgip->getCommand() == GIP_VIRTUAL_KEYCODE) {
            const uint8_t *vk_data = device->incoming_xgip->getData();
            uint16_t vk_len = device->incoming_xgip->getDataLength();
#if GIP_TRACE_ENABLED
            printf("GIP: VIRTUAL_KEYCODE received [len=%d]:", vk_len);
            for (int i = 0; i < vk_len; i++) printf(" %02X", vk_data[i]);
            printf("\n");
#endif
            if (vk_len >= 2 && (vk_data[1] == 0x5B || vk_data[0] == 0x5B)) { // 0x5B = GIP_VKEY_LEFT_WIN (Guide button)
                device->has_virtual_key_guide = true;
                device->virtual_key_guide = (vk_data[1] == 0x5B) ? (vk_data[0] ? 1 : 0) : (vk_data[1] ? 1 : 0);
                auto gamepad = (XboxOneGamepad_Data_t *)device->raw_input;
                gamepad->guide = device->virtual_key_guide;
#if GIP_TRACE_ENABLED
                printf("GIP: Virtual Key Guide button %s\n", device->virtual_key_guide ? "PRESSED" : "RELEASED");
#endif
            }
        }
#if GIP_TRACE_ENABLED
        else if (device->incoming_xgip->getCommand() == GIP_KEEPALIVE) {
            const uint8_t *st_data = device->incoming_xgip->getData();
            uint16_t st_len = device->incoming_xgip->getDataLength();
            printf("GIP Status/Keepalive 0x03 [len=%d]:", st_len);
            for (int i = 0; i < st_len; i++) printf(" %02X", st_data[i]);
            printf("\n");
        }
#endif

        // Process packet using device interface
        gip_process_packet(device->incoming_xgip, device);

        data += wire_len;
        len -= wire_len;
        processed_any = true;
    }

    return processed_any;
}

bool gip_device_generate_ack(
    gip_device_t *device,
    uint8_t **out_data,
    uint16_t *out_len)
{
    if (!device || !device->incoming_xgip || !out_data || !out_len) {
        return false;
    }
    
    if (!device->incoming_xgip->ackRequired()) {
        return false;
    }
    
    *out_data = device->incoming_xgip->generateAckPacket();
    *out_len = device->incoming_xgip->getPacketLength();
    
    return true;
}

void gip_device_set_ack_wait(gip_device_t *device, uint32_t timestamp)
{
    if (!device) return;
    
    device->waiting_ack = true;
    device->waiting_ack_timeout = timestamp;
}

bool gip_device_ack_timeout(gip_device_t *device, uint32_t current_time, uint32_t timeout_ms)
{
    if (!device || !device->waiting_ack) {
        return false;
    }
    
    return (current_time - device->waiting_ack_timeout) >= timeout_ms;
}

void gip_device_clear_ack_wait(gip_device_t *device)
{
    if (!device) return;
    
    device->waiting_ack = false;
}

void gip_device_update(
    gip_device_t *device,
    uint32_t current_time,
    uint32_t ack_timeout_ms,
    void (*queue_packet)(void *context, const uint8_t *data, uint16_t len),
    void *context)
{
    if (!device || !queue_packet) {
        return;
    }
    
    // Check ACK timeout
    if (gip_device_ack_timeout(device, current_time, ack_timeout_ms)) {
        gip_device_clear_ack_wait(device);
    }
    
    // MS-GIPUSB "Reliable Message Acknowledgement": while a chunked transfer is
    // incomplete, nudge the sender with periodic ACKs of contiguous bytes received,
    // and abandon/re-request the transfer if no data arrives within the timeout.
    if (device->incoming_chunk_pending) {
        if (device->incoming_chunk_last_data_at == 0) {
            device->incoming_chunk_last_data_at = current_time;
            device->incoming_chunk_last_ack_at = current_time;
        }
        
        if ((current_time - device->incoming_chunk_last_data_at) >= GIP_RELIABLE_MESSAGE_TIMEOUT_MS) {
            printf("GIP: reliable transfer timed out, retrying identify request\n");
            device->incoming_chunk_pending = false;
            device->incoming_chunk_last_data_at = 0;
            device->incoming_chunk_last_ack_at = 0;
            device->incoming_chunk_heartbeat_acks = 0;
            if (device->incoming_xgip) {
                device->incoming_xgip->reset();
            }
            gip_request_device_descriptor(device);
        }
        else if (device->incoming_chunk_heartbeat_acks < GIP_RELIABLE_MESSAGE_MAX_HEARTBEAT_ACKS &&
                 (current_time - device->incoming_chunk_last_ack_at) >= GIP_RELIABLE_MESSAGE_ACK_INTERVAL_MS)
        {
            uint8_t *ack_data = device->incoming_xgip->generateAckPacket();
            uint16_t ack_len = device->incoming_xgip->getPacketLength();
            queue_packet(context, ack_data, ack_len);
            device->incoming_chunk_last_ack_at = current_time;
            device->incoming_chunk_heartbeat_acks++;
        }
    }
    
    // Do not send new packets until ACK returns
    if (device->waiting_ack) {
        return;
    }
    
    // Send outgoing packet if waiting
    if (device->outgoing_xgip && device->outgoing_xgip->waitingToSend()) {
        if (device->outgoing_xgip->getSequence() == 0) {
            uint8_t seq = gip_sequence_pool_next(&device->tx_sequence_pools, device->outgoing_xgip->getCommand());
            device->outgoing_xgip->setSequence(seq);
        }
        uint8_t *packet = device->outgoing_xgip->generatePacket();
        uint16_t packet_len = device->outgoing_xgip->getPacketLength();
        
        // Queue packet for transmission
        queue_packet(context, packet, packet_len);
        
        // Set ACK wait if packet requires ACK
        if (device->outgoing_xgip->getPacketAck() == 1) {
            gip_device_set_ack_wait(device, current_time);
        }
    }
}

// Callback for report queue
static void gip_queue_to_report_queue(void *context, const uint8_t *data, uint16_t len)
{
    gip_report_queue_t *queue = (gip_report_queue_t *)context;
    gip_report_queue_push(queue, data, len);
}

void gip_device_update_with_queue(
    gip_device_t *device,
    uint32_t current_time,
    uint32_t ack_timeout_ms,
    gip_report_queue_t *report_queue)
{
    gip_device_update(device, current_time, ack_timeout_ms, 
                      gip_queue_to_report_queue, report_queue);
}
