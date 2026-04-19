#include "bt/device/bt_descriptors.h"
#include <stdint.h>
#include "usb/device/hid_device.h"
#include "hid_reports.h"

// Dance pads really need simultaneous directions, so they emulate buttons instead of hats
uint8_t const desc_hid_report_buttons[] =
    {TUD_HID_REPORT_DESC_GAME_CONTROLLER(HID_REPORT_ID(ReportIdGamepad), TUD_HID_REPORT_DESC_GAME_CONTROLLER_BUTTONS)};

// for compatibility though, report the dpad as a hat for non-dancepad devices
uint8_t const desc_hid_report_hat[] =
    {TUD_HID_REPORT_DESC_GAME_CONTROLLER(HID_REPORT_ID(ReportIdGamepad), TUD_HID_REPORT_DESC_GAME_CONTROLLER_HAT_SWITCH)};

uint8_t const desc_hid_report_keyboard[] =
    {TUD_HID_REPORT_DESC_KEYBOARD_NKRO(HID_REPORT_ID(ReportIdGamepad))};