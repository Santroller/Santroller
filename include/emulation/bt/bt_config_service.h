#pragma once
#include <stdint.h>

// Exposes the USB config interface (the vendor HID collection the configurator uses) as a second
// HID service over BLE. ATT requests are queued from the bluetooth stack and handed to
// HIDConfigDevice from the main loop, with the ATT response held back until they are handled.

// The Web Bluetooth config service (bt_config.gatt, 53414E54-524F-4C4C-4552-000000000000), little endian. It's
// advertised so the configurator, and other Santrollers, can recognise us whatever the device is named
static const uint8_t santroller_config_service_uuid_le[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x52, 0x45,
                                                              0x4C, 0x4C, 0x4F, 0x52, 0x54, 0x4E, 0x41, 0x53};

// Registers the service with the ATT server. Call with the bluetooth stack lock held, after att_server_init.
// hid_start / hid_end is the config HID service in the current GATT database, the Web Bluetooth service is found by UUID
void bt_config_service_init(uint16_t hid_start, uint16_t hid_end);
// Forgets the registration, call before att_server_deinit so the next init can use a different GATT database
void bt_config_service_deinit();
// The GATT database used while no profile is using bluetooth, and its config HID service
const uint8_t *bt_config_only_profile(uint16_t *hid_start, uint16_t *hid_end);
// Services queued requests, call from the main loop
void bt_config_service_process(bool full_poll, bool send_events);
// Only hands queued reads and writes to HIDConfigDevice, for while the rest of the device is held off
// (a config upload), as each write's ATT response waits on it
void bt_config_service_handle_requests();
// Call from the bluetooth stack when the connection drops
void bt_config_service_disconnected();

// Used by HIDConfigDevice to send config events over bluetooth
bool bt_config_connected();
bool bt_config_can_send_event();
uint16_t bt_config_max_event_size();
bool bt_config_send_event(const uint8_t *data, uint16_t len);

// Implemented by HIDConfigDevice. Kept free of USB / bluetooth types, as both stacks define hid_report_type_t.
// Buffers include the report id, the same as over USB.
const uint8_t *hid_config_report_descriptor(uint16_t *len);
uint16_t hid_config_bt_get_report(uint8_t report_id, uint8_t report_type, uint8_t *buffer, uint16_t reqlen);
// Returns false if the report was refused, which is passed back to the host as an ATT error
bool hid_config_bt_set_report(uint8_t report_id, uint8_t report_type, const uint8_t *buffer, uint16_t len);
// Processes the config device when it isn't already a USB instance
void hid_config_bt_process(bool full_poll, bool send_events);
