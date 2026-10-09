#pragma once
// The HID report descriptors the firmware advertises, built from the same hid_reports.h macros with the
// same arguments as the device sources (most of those arrays are file static, in .cpp files that need
// the whole USB stack). Each one names the source line it mirrors, and descriptor_sources_test.cpp checks
// the source still instantiates that macro.
//
// Bluetooth's descriptors (bt_descriptors.cpp) are compiled into the test from the real source instead,
// and the Spice2x one comes from the real spice2x_device.cpp.
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include "hid_reports.h"
#include "emulation/keyboard_mouse.hpp"
#include "emulation/bt/bt_descriptors.h"

namespace hid_desc
{

// Strings the config descriptor sizes two reports from (GIT_HASH and PICO_BOARD in the firmware build).
// Their values don't matter to the tests, only that the descriptor and get_report use the same sizes.
inline constexpr char kConfigVersion[] = "1106b22d";
inline constexpr char kConfigType[] = "pico_w_compat";
inline constexpr uint16_t kFirmwareUploadReportSize = 63;

// src/emulation/usb/hid_gamepad_device.cpp:24-35
inline constexpr uint8_t kHidGamepadButtons[] = {
    TUD_HID_REPORT_DESC_GAME_CONTROLLER(HID_REPORT_ID(ReportIdGamepad), TUD_HID_REPORT_DESC_GAME_CONTROLLER_BUTTONS)};
inline constexpr uint8_t kHidGamepadHat[] = {
    TUD_HID_REPORT_DESC_GAME_CONTROLLER(HID_REPORT_ID(ReportIdGamepad), TUD_HID_REPORT_DESC_GAME_CONTROLLER_HAT_SWITCH)};
inline constexpr uint8_t kHidGamepadButtonsBattery[] = {
    TUD_HID_REPORT_DESC_GAME_CONTROLLER(HID_REPORT_ID(ReportIdGamepad), TUD_HID_REPORT_DESC_GAME_CONTROLLER_BUTTONS, TUD_HID_REPORT_DESC_BATTERY())};
inline constexpr uint8_t kHidGamepadHatBattery[] = {
    TUD_HID_REPORT_DESC_GAME_CONTROLLER(HID_REPORT_ID(ReportIdGamepad), TUD_HID_REPORT_DESC_GAME_CONTROLLER_HAT_SWITCH, TUD_HID_REPORT_DESC_BATTERY())};

// src/emulation/usb/hid_keyboard_device.cpp:18-20
inline constexpr uint8_t kHidKeyboard[] = {TUD_HID_REPORT_DESC_KEYBOARD_10KRO(HID_REPORT_ID(KEYBOARD_REPORT_ID)),
                                           TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(MOUSE_REPORT_ID)),
                                           TUD_HID_REPORT_DESC_CONSUMER_MULTI(HID_REPORT_ID(CONSUMER_REPORT_ID))};

// src/emulation/usb/ps3_device.cpp:167-172
inline constexpr uint8_t kPs3ThirdParty[] = {TUD_HID_REPORT_DESC_PS3_THIRDPARTY_GAMEPAD()};
inline constexpr uint8_t kPs3FirstParty[] = {TUD_HID_REPORT_DESC_PS3_FIRSTPARTY_GAMEPAD(HID_REPORT_ID(ReportIdGamepad))};

// src/emulation/usb/ps4_device.cpp:47-49
inline constexpr uint8_t kPs4[] = {TUD_HID_REPORT_DESC_PS4_THIRDPARTY_GAMEPAD(HID_REPORT_ID(ReportIdGamepad))};

// src/emulation/usb/ps5_device.cpp:44-46
inline constexpr uint8_t kPs5[] = {TUD_HID_REPORT_DESC_PS5_THIRDPARTY_GAMEPAD(HID_REPORT_ID(ReportIdGamepad))};

// src/emulation/usb/switch_device.cpp:15-18
inline constexpr uint8_t kSwitchPro[] = {TUD_HID_REPORT_SWITCH()};
inline constexpr uint8_t kSwitchArcade[] = {TUD_HID_REPORT_DESC_SWITCH_ARCADE()};

// src/emulation/usb/gh_arcade_device.cpp:12
inline constexpr uint8_t kGhArcade[] = {TUD_HID_REPORT_DESC_GUITAR_HERO_ARCADE()};

// src/emulation/usb/hid_config_device.cpp:34-54
inline constexpr uint8_t kConfig[] = {
    HID_USAGE_PAGE_N(HID_USAGE_PAGE_VENDOR, 2),
    HID_USAGE(0x01),
    HID_COLLECTION(HID_COLLECTION_APPLICATION),
    TUD_HID_REPORT_DESC_GENERIC_INFEATURE(63, HID_REPORT_ID(ReportIdConfig)),
    TUD_HID_REPORT_DESC_GENERIC_FEATURE(63, HID_REPORT_ID(ReportIdConfigInfo)),
    TUD_HID_REPORT_DESC_GENERIC_FEATURE(63, HID_REPORT_ID(ReportIdConfigInfoSaved)),
    TUD_HID_REPORT_DESC_GENERIC_FEATURE(1, HID_REPORT_ID(ReportIdLoaded)),
    TUD_HID_REPORT_DESC_GENERIC_FEATURE(63, HID_REPORT_ID(ReportIdCommand)),
    TUD_HID_REPORT_DESC_GENERIC_FEATURE(1, HID_REPORT_ID(ReportIdKeepalive)),
    TUD_HID_REPORT_DESC_GENERIC_FEATURE(1, HID_REPORT_ID(ReportIdBootloader)),
    TUD_HID_REPORT_DESC_GENERIC_FEATURE(63, HID_REPORT_ID(ReportIdGetActiveProfiles)),
    TUD_HID_REPORT_DESC_GENERIC_FEATURE(63, HID_REPORT_ID(ReportIdUpdateFirmware)),
    TUD_HID_REPORT_DESC_GENERIC_FEATURE(kFirmwareUploadReportSize, HID_REPORT_ID(ReportIdUploadFirmware)),
    TUD_HID_REPORT_DESC_GENERIC_OUTPUT(kFirmwareUploadReportSize, HID_REPORT_ID(ReportIdUploadFirmware)),
    TUD_HID_REPORT_DESC_GENERIC_FEATURE(sizeof(kConfigVersion) + 1, HID_REPORT_ID(ReportIdGetVersion)),
    TUD_HID_REPORT_DESC_GENERIC_FEATURE(sizeof(kConfigType) + 1, HID_REPORT_ID(ReportIdGetType)),
    HID_COLLECTION_END};

enum class Transport
{
    Usb,
    Bluetooth,
};

// A descriptor plus where it comes from, for the checks every descriptor gets
struct NamedDescriptor
{
    std::string name;
    std::vector<uint8_t> bytes;
    Transport transport;
    // USB descriptors served from a .cpp the test can't build: the file and the instantiation in it
    std::string source;
    std::string snippet;
};

template <size_t N>
std::vector<uint8_t> bytes_of(const uint8_t (&desc)[N])
{
    return std::vector<uint8_t>(desc, desc + N);
}

// Spice2x's descriptor, as Spice2xDevice::initialize() builds it (string indices patched in)
std::vector<uint8_t> spice2x_descriptor();

inline std::vector<NamedDescriptor> all_descriptors()
{
    return {
        {"HidGamepadButtons", bytes_of(kHidGamepadButtons), Transport::Usb, "src/emulation/usb/hid_gamepad_device.cpp",
         "desc_hid_report_buttons[]={TUD_HID_REPORT_DESC_GAME_CONTROLLER(HID_REPORT_ID(ReportIdGamepad),TUD_HID_REPORT_DESC_GAME_CONTROLLER_BUTTONS)};"},
        {"HidGamepadHat", bytes_of(kHidGamepadHat), Transport::Usb, "src/emulation/usb/hid_gamepad_device.cpp",
         "desc_hid_report_hat[]={TUD_HID_REPORT_DESC_GAME_CONTROLLER(HID_REPORT_ID(ReportIdGamepad),TUD_HID_REPORT_DESC_GAME_CONTROLLER_HAT_SWITCH)};"},
        {"HidGamepadButtonsBattery", bytes_of(kHidGamepadButtonsBattery), Transport::Usb, "src/emulation/usb/hid_gamepad_device.cpp",
         "desc_hid_report_buttons_battery[]={TUD_HID_REPORT_DESC_GAME_CONTROLLER(HID_REPORT_ID(ReportIdGamepad),TUD_HID_REPORT_DESC_GAME_CONTROLLER_BUTTONS,TUD_HID_REPORT_DESC_BATTERY())};"},
        {"HidGamepadHatBattery", bytes_of(kHidGamepadHatBattery), Transport::Usb, "src/emulation/usb/hid_gamepad_device.cpp",
         "desc_hid_report_hat_battery[]={TUD_HID_REPORT_DESC_GAME_CONTROLLER(HID_REPORT_ID(ReportIdGamepad),TUD_HID_REPORT_DESC_GAME_CONTROLLER_HAT_SWITCH,TUD_HID_REPORT_DESC_BATTERY())};"},
        {"HidKeyboard", bytes_of(kHidKeyboard), Transport::Usb, "src/emulation/usb/hid_keyboard_device.cpp",
         "desc_hid_keyboard_report[]={TUD_HID_REPORT_DESC_KEYBOARD_10KRO(HID_REPORT_ID(KEYBOARD_REPORT_ID)),TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(MOUSE_REPORT_ID)),TUD_HID_REPORT_DESC_CONSUMER_MULTI(HID_REPORT_ID(CONSUMER_REPORT_ID))};"},
        {"Ps3ThirdParty", bytes_of(kPs3ThirdParty), Transport::Usb, "src/emulation/usb/ps3_device.cpp",
         "desc_hid_report_ps3_thirdparty[]={TUD_HID_REPORT_DESC_PS3_THIRDPARTY_GAMEPAD()};"},
        {"Ps3FirstParty", bytes_of(kPs3FirstParty), Transport::Usb, "src/emulation/usb/ps3_device.cpp",
         "desc_hid_report_ps3_gamepad[]={TUD_HID_REPORT_DESC_PS3_FIRSTPARTY_GAMEPAD(HID_REPORT_ID(ReportIdGamepad))};"},
        {"Ps4", bytes_of(kPs4), Transport::Usb, "src/emulation/usb/ps4_device.cpp",
         "desc_hid_report_ps4[]={TUD_HID_REPORT_DESC_PS4_THIRDPARTY_GAMEPAD(HID_REPORT_ID(ReportIdGamepad))};"},
        {"Ps5", bytes_of(kPs5), Transport::Usb, "src/emulation/usb/ps5_device.cpp",
         "desc_hid_report_ps5[]={TUD_HID_REPORT_DESC_PS5_THIRDPARTY_GAMEPAD(HID_REPORT_ID(ReportIdGamepad))};"},
        {"SwitchPro", bytes_of(kSwitchPro), Transport::Usb, "src/emulation/usb/switch_device.cpp",
         "desc_hid_report_switch_pro[]={TUD_HID_REPORT_SWITCH()};"},
        {"SwitchArcade", bytes_of(kSwitchArcade), Transport::Usb, "src/emulation/usb/switch_device.cpp",
         "desc_hid_report_switch_arcade[]={TUD_HID_REPORT_DESC_SWITCH_ARCADE()};"},
        {"GhArcade", bytes_of(kGhArcade), Transport::Usb, "src/emulation/usb/gh_arcade_device.cpp",
         "desc_hid_report_arcade[]={TUD_HID_REPORT_DESC_GUITAR_HERO_ARCADE()};"},
        {"Config", bytes_of(kConfig), Transport::Usb, "src/emulation/usb/hid_config_device.cpp",
         "TUD_HID_REPORT_DESC_GENERIC_INFEATURE(63,HID_REPORT_ID(ReportIdConfig)),"
         "TUD_HID_REPORT_DESC_GENERIC_FEATURE(63,HID_REPORT_ID(ReportIdConfigInfo)),"
         "//thesaved(ratherthanrunning)configinfo,answeredbythesamecopy_config_info"
         "TUD_HID_REPORT_DESC_GENERIC_FEATURE(63,HID_REPORT_ID(ReportIdConfigInfoSaved)),"
         "TUD_HID_REPORT_DESC_GENERIC_FEATURE(1,HID_REPORT_ID(ReportIdLoaded)),"
         "TUD_HID_REPORT_DESC_GENERIC_FEATURE(63,HID_REPORT_ID(ReportIdCommand)),"
         "TUD_HID_REPORT_DESC_GENERIC_FEATURE(1,HID_REPORT_ID(ReportIdKeepalive)),"
         "TUD_HID_REPORT_DESC_GENERIC_FEATURE(1,HID_REPORT_ID(ReportIdBootloader)),"
         "TUD_HID_REPORT_DESC_GENERIC_FEATURE(63,HID_REPORT_ID(ReportIdGetActiveProfiles)),"
         "TUD_HID_REPORT_DESC_GENERIC_FEATURE(63,HID_REPORT_ID(ReportIdUpdateFirmware)),"
         "TUD_HID_REPORT_DESC_GENERIC_FEATURE(firmware_upload_report_size,HID_REPORT_ID(ReportIdUploadFirmware)),"
         "TUD_HID_REPORT_DESC_GENERIC_OUTPUT(firmware_upload_report_size,HID_REPORT_ID(ReportIdUploadFirmware)),"
         "TUD_HID_REPORT_DESC_GENERIC_FEATURE(sizeof(version)+1,HID_REPORT_ID(ReportIdGetVersion)),"
         "TUD_HID_REPORT_DESC_GENERIC_FEATURE(sizeof(type)+1,HID_REPORT_ID(ReportIdGetType)),"
         "HID_COLLECTION_END};"},
        {"Spice2x", spice2x_descriptor(), Transport::Usb, "", ""},
        {"BtGamepadButtons", bytes_of(desc_hid_report_buttons), Transport::Bluetooth, "", ""},
        {"BtGamepadHat", bytes_of(desc_hid_report_hat), Transport::Bluetooth, "", ""},
        {"BtKeyboard", std::vector<uint8_t>(desc_hid_report_keyboard, desc_hid_report_keyboard + desc_hid_report_keyboard_len),
         Transport::Bluetooth, "", ""},
    };
}

} // namespace hid_desc
