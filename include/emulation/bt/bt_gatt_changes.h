#pragma once
#include <stdint.h>

// Each bluetooth mode (gamepad, keyboard, config only) has its own GATT database, laid out differently.
// Hosts like Windows cache the layout of a bonded device and keep using it until they get a Service
// Changed indication, reading the wrong attributes otherwise. So this remembers which database each
// bonded host last saw, and tells it to re-discover when that changed.

// The Generic Attribute service sits straight after the GAP service in every database, so these are
// the same in all of them (checked where each generated database is included)
#define BT_GATT_SERVICE_START_HANDLE 0x0004
#define BT_GATT_SERVICE_END_HANDLE 0x0009
#define BT_GATT_SERVICE_CHANGED_VALUE_HANDLE 0x0006
#define BT_GATT_SERVICE_CHANGED_CLIENT_CONFIGURATION_HANDLE 0x0007

// Call with the bluetooth stack lock held, after att_server_init with the database in use
void bt_gatt_changes_init();
// Call before att_server_deinit
void bt_gatt_changes_deinit();
