#pragma once

#include <stdint.h>
#include "enums.pb.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * GIP Device Interface
 * 
 * Callbacks for handling GIP packets that require device-specific logic.
 */
typedef struct gip_device_interface {
    // Called when device descriptor is received
    void (*on_device_descriptor)(void *user_context, SubType subtype);
    
    // Called when arrival packet is received
    void (*on_arrival)(void *user_context);
    
    // Called to queue outgoing packet
    void (*queue_packet)(void *user_context, const uint8_t *data, uint16_t len);
    
    // Called to send an immediate ACK packet with high priority
    void (*send_ack)(void *user_context, const uint8_t *data, uint16_t len);

    // Called when device reports disconnection (e.g. via GIP_STATUS keepalive)
    void (*on_disconnect)(void *user_context);

    // Called with packets from a device plugged into the controller (a non-zero client id), e.g. a chatpad.
    // Returns how many bytes the packet took up, or 0 to drop the rest
    uint16_t (*process_secondary)(void *user_context, const uint8_t *data, uint16_t len);
} gip_device_interface_t;

#ifdef __cplusplus
}
#endif
