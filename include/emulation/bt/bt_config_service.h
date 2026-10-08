#pragma once
#include <stdint.h>

// Exposes the USB config interface (the vendor HID collection the configurator uses) as a second
// HID service over BLE. ATT requests are queued from the bluetooth stack and handed to
// HIDConfigDevice from the main loop, with the ATT response held back until they are handled.

// Registers the service with the ATT server. Call with the bluetooth stack lock held, after att_server_init
void bt_config_service_init();
// Services queued requests, call from the main loop
void bt_config_service_process(bool full_poll, bool send_events);
// Call from the bluetooth stack when the connection drops
void bt_config_service_disconnected();

// Used by HIDConfigDevice to send config events over bluetooth
bool bt_config_can_send_event();
uint16_t bt_config_max_event_size();
bool bt_config_send_event(const uint8_t *data, uint16_t len);

// Implemented by HIDConfigDevice. Kept free of USB / bluetooth types, as both stacks define hid_report_type_t.
// Buffers include the report id, the same as over USB.
const uint8_t *hid_config_report_descriptor(uint16_t *len);
uint16_t hid_config_bt_get_report(uint8_t report_id, uint8_t report_type, uint8_t *buffer, uint16_t reqlen);
void hid_config_bt_set_report(uint8_t report_id, uint8_t report_type, const uint8_t *buffer, uint16_t len);
// Processes the config device when it isn't already a USB instance
void hid_config_bt_process(bool full_poll, bool send_events);
