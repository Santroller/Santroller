// The generated GATT databases all define profile_data and the same handle macros, so each one lives in
// its own translation unit
#include "emulation/bt/bt_config_profile.h"
#include "emulation/bt/bt_config_service.h"

const uint8_t *bt_config_only_profile(uint16_t *hid_start, uint16_t *hid_end)
{
    *hid_start = ATT_SERVICE_ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE_START_HANDLE;
    *hid_end = ATT_SERVICE_ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE_END_HANDLE;
    return profile_data;
}
