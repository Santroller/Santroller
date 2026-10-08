#pragma once
int ble_main(void);
void ble_stop_scan();
void ble_start_scan();
bool ble_is_connecting();
bool ble_has_connected_device();
// Scanning in the background to reconnect paired BLE controllers
bool ble_background_scan_active();
// Scanning for new BLE controllers to pair
bool ble_discovery_active();
void ble_tick();
// re-evaluate the background reconnect, e.g. when classic connections come or go
void ble_resync_reconnect();
// disconnect a controller on purpose, holding off reconnecting to it for a while
void ble_disconnect(const uint8_t addr[6]);