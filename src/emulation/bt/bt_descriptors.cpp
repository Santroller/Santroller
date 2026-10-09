#include "emulation/bt/bt_descriptors.h"
#include <stdint.h>
#include "emulation/usb/hid_device.h"
#include "hid_reports.h"
#include "class/hid/hid_device.h"
#include "emulation/keyboard_mouse.hpp"

// Dance pads really need simultaneous directions, so they emulate buttons instead of hats
uint8_t const desc_hid_report_buttons[] =
    {TUD_HID_REPORT_DESC_GAME_CONTROLLER(HID_REPORT_ID(ReportIdGamepad), TUD_HID_REPORT_DESC_GAME_CONTROLLER_BUTTONS)};

// for compatibility though, report the dpad as a hat for non-dancepad devices
uint8_t const desc_hid_report_hat[] =
    {TUD_HID_REPORT_DESC_GAME_CONTROLLER(HID_REPORT_ID(ReportIdGamepad), TUD_HID_REPORT_DESC_GAME_CONTROLLER_HAT_SWITCH)};

// Same layout as the USB keyboard: a keyboard, a mouse and media keys, told apart by report id
uint8_t const desc_hid_report_keyboard[] =
    {TUD_HID_REPORT_DESC_KEYBOARD_10KRO(HID_REPORT_ID(KEYBOARD_REPORT_ID)),
     TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(MOUSE_REPORT_ID)),
     TUD_HID_REPORT_DESC_CONSUMER_MULTI(HID_REPORT_ID(CONSUMER_REPORT_ID))};
const uint16_t desc_hid_report_keyboard_len = sizeof(desc_hid_report_keyboard);

// The generated GATT databases all define profile_data and the same handle macros, so each one lives in its
// own translation unit. The keyboard one lives here, next to the report map it serves (bt_gamepad.cpp has
// the gamepad one, bt_config_profile.cpp the config only one).
#include "emulation/bt/bt_keyboard_profile.h"

const uint8_t *bt_keyboard_profile(uint16_t *config_start, uint16_t *config_end)
{
    *config_start = ATT_SERVICE_ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE_02_START_HANDLE;
    *config_end = ATT_SERVICE_ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE_02_END_HANDLE;
    return profile_data;
}
