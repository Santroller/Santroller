#ifndef WIRELESS_H
#define WIRELESS_H

#include <stdint.h>
#include <stdbool.h>

// Task function for periodic processing (e.g., pairing timeout)
void wireless_task(void);

// Process incoming wireless data from MT76
void wireless_process_data(const uint8_t *data, uint16_t len);

// Process wireless packet (802.11 frame)
void wireless_process_packet(const uint8_t *data, uint16_t len);

// Handle controller association request
void wireless_handle_association(const uint8_t *addr);

#endif // WIRELESS_H
