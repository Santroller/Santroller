// The generated GATT databases all define profile_data and the same handle macros, so each one lives in
// its own translation unit
#include "emulation/bt/bt_config_profile.h"
#include "emulation/bt/bt_config_service.h"
#include "emulation/bt/bt_gatt_changes.h"

// bt_gatt_changes relies on the Generic Attribute service being at the same handles in every database
static_assert(ATT_SERVICE_GATT_SERVICE_START_HANDLE == BT_GATT_SERVICE_START_HANDLE, "GATT service moved");
static_assert(ATT_SERVICE_GATT_SERVICE_END_HANDLE == BT_GATT_SERVICE_END_HANDLE, "GATT service moved");
static_assert(ATT_CHARACTERISTIC_GATT_SERVICE_CHANGED_01_VALUE_HANDLE == BT_GATT_SERVICE_CHANGED_VALUE_HANDLE, "Service Changed moved");
static_assert(ATT_CHARACTERISTIC_GATT_SERVICE_CHANGED_01_CLIENT_CONFIGURATION_HANDLE == BT_GATT_SERVICE_CHANGED_CLIENT_CONFIGURATION_HANDLE, "Service Changed moved");

const uint8_t *bt_config_only_profile(uint16_t *hid_start, uint16_t *hid_end)
{
    *hid_start = ATT_SERVICE_ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE_START_HANDLE;
    *hid_end = ATT_SERVICE_ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE_END_HANDLE;
    return profile_data;
}
