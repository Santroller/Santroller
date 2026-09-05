#include "gip_device.h"
#include "gip_packet_handler.h"
#include "gip_report_queue.h"
#include "../../lib/xgip_protocol/xgip_protocol.h"
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
    
    // Feed data to incoming XGIP protocol parser
    if (!device->incoming_xgip->parse((uint8_t *)data, len)) {
        return false;  // Failed to parse
    }
#if GIP_TRACE_ENABLED
    printf("gip_device_process_incoming: parsed successfully, cmd=0x%02X\n", device->incoming_xgip->getCommand());
#endif
    
    if (device->incoming_xgip->getChunked()) {
        if (!device->incoming_xgip->endOfChunk()) {
            // Mid-transfer chunk fragment; accumulate and wait for the terminating chunk.
            // Timestamps are (re)established on the next gip_device_update() tick.
            device->incoming_chunk_pending = true;
            device->incoming_chunk_last_data_at = 0;
            device->incoming_chunk_heartbeat_acks = 0;
            return true;
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
        }
    }
    
    // Process packet using device interface
    gip_process_packet(device->incoming_xgip, device);
    
    return true;
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
