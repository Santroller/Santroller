#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "gip_device_interface.h"

// Forward declaration
typedef struct XGIPProtocol XGIPProtocol;

// GIP device state
typedef struct gip_device_t {
    XGIPProtocol *incoming_xgip;  // For parsing incoming packets
    XGIPProtocol *outgoing_xgip;  // For generating outgoing packets
    uint8_t raw_input[64];        // Raw GIP input data (XboxOneGamepad_Data_t, etc.)
    SubType subtype;              // Device subtype
    bool waiting_ack;             // Waiting for ACK response
    uint32_t waiting_ack_timeout; // ACK timeout timestamp
    // Incoming chunked reliable-transfer tracking, per MS-GIPUSB "Reliable Message Acknowledgement"
    bool incoming_chunk_pending;           // Waiting on remaining fragments of a chunked reliable transfer
    uint32_t incoming_chunk_last_data_at;  // Timestamp (ms) of the last received fragment, 0 = not yet timestamped
    uint32_t incoming_chunk_last_ack_at;   // Timestamp (ms) the last ACK was sent for this transfer
    uint8_t incoming_chunk_heartbeat_acks; // Heartbeat ACKs sent since the last received fragment
    void *user_context;           // User context (e.g., pointer to owning controller instance)
    const gip_device_interface_t *interface;  // Interface for callbacks
} gip_device_t;

/**
 * Initialize a GIP device
 * 
 * @param device GIP device to initialize
 */
void gip_device_init(gip_device_t *device);

/**
 * Cleanup a GIP device
 * 
 * @param device GIP device to cleanup
 */
void gip_device_cleanup(gip_device_t *device);

/**
 * Process incoming GIP data
 * 
 * Uses the interface set in device->interface for callbacks.
 * 
 * @param device GIP device (must have interface set)
 * @param data Raw packet data
 * @param len Length of data
 * @return true if packet was processed, false if waiting for more data
 */
bool gip_device_process_incoming(
    gip_device_t *device,
    const uint8_t *data,
    uint16_t len);

/**
 * Generate an ACK packet if needed
 * 
 * @param device GIP device
 * @param out_data Output buffer for ACK packet
 * @param out_len Output length of ACK packet
 * @return true if ACK was generated, false otherwise
 */
bool gip_device_generate_ack(
    gip_device_t *device,
    uint8_t **out_data,
    uint16_t *out_len
);

/**
 * Set ACK wait state
 * 
 * @param device GIP device
 * @param timestamp Current timestamp in milliseconds
 */
void gip_device_set_ack_wait(gip_device_t *device, uint32_t timestamp);

/**
 * Check if ACK wait has timed out
 * 
 * @param device GIP device
 * @param current_time Current timestamp in milliseconds
 * @param timeout_ms Timeout in milliseconds
 * @return true if timed out, false otherwise
 */
bool gip_device_ack_timeout(gip_device_t *device, uint32_t current_time, uint32_t timeout_ms);

/**
 * Clear ACK wait state
 * 
 * @param device GIP device
 */
void gip_device_clear_ack_wait(gip_device_t *device);

/**
 * Update GIP device - handles ACK timeout and outgoing packets
 * 
 * @param device GIP device to update
 * @param current_time Current time in milliseconds
 * @param ack_timeout_ms ACK timeout in milliseconds
 * @param queue_packet Callback to queue outgoing packet for transmission
 * @param context Context to pass to queue_packet callback
 */
void gip_device_update(
    gip_device_t *device,
    uint32_t current_time,
    uint32_t ack_timeout_ms,
    void (*queue_packet)(void *context, const uint8_t *data, uint16_t len),
    void *context
);

// Forward declaration for report queue
typedef struct gip_report_queue gip_report_queue_t;

/**
 * Update GIP device with shared report queue
 * Convenience function that queues packets directly to a report queue
 * 
 * @param device GIP device to update
 * @param current_time Current time in milliseconds
 * @param ack_timeout_ms ACK timeout in milliseconds
 * @param report_queue Report queue to push packets to
 */
void gip_device_update_with_queue(
    gip_device_t *device,
    uint32_t current_time,
    uint32_t ack_timeout_ms,
    gip_report_queue_t *report_queue
);

#ifdef __cplusplus
}
#endif
