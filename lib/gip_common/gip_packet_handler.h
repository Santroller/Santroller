#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "protocols/xbox_gip.h"

// Forward declarations
typedef struct XGIPProtocol XGIPProtocol;
typedef struct gip_device_t gip_device_t;

// Device type mapping structure
typedef struct {
    uint8_t subtype;
    const char *name;
} gip_device_type_mapping_t;

/**
 * Shared default callback implementations
 */

// Default ACK callback - clears ACK wait state
void gip_default_ack_callback(void *context);

// Default auth callback - sends auth complete
void gip_default_auth_callback(void *context, const uint8_t *data, uint16_t len);

// Default arrival callback - requests device descriptor
void gip_default_arrival_callback(void *context, void (*queue_packet)(void *, const uint8_t *, uint16_t));

/**
 * Process a received GIP packet
 * 
 * @param xgip XGIPProtocol instance (already parsed)
 * @param device GIP device
 * @return true if packet was handled, false otherwise
 */
bool gip_process_packet(XGIPProtocol *xgip, gip_device_t *device);

/**
 * Detect device subtype from device descriptor data
 * 
 * @param data Device descriptor data (after BinaryMetadataHeader)
 * @param len Length of data
 * @param mappings Array of device type mappings
 * @param mapping_count Number of mappings
 * @return Detected subtype, or 0xFF if unknown
 */
uint8_t gip_detect_device_subtype(
    const uint8_t *data,
    uint16_t len,
    const gip_device_type_mapping_t *mappings,
    size_t mapping_count
);

/**
 * Send power-on sequence to device
 * 
 * @param device GIP device
 */
void gip_send_power_on_sequence(gip_device_t *device);

/**
 * Request device descriptor
 * 
 * @param device GIP device
 */
void gip_request_device_descriptor(gip_device_t *device);

/**
 * Send authentication complete packet
 * 
 * @param device GIP device
 */
void gip_send_auth_complete(gip_device_t *device);

#ifdef __cplusplus
}
#endif
