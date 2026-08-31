#pragma once

#include <stdint.h>
#include "../../build/proto/enums.pb.h"

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
} gip_device_interface_t;

#ifdef __cplusplus
}
#endif
