#include "devices/bt/bt_classic_host.hpp"
#include "hidparser.h"
#include "btstack.h"
#include "emulation/usb/usb_devices.h"

#include <memory>

// ============================================================================
// Factory
// ============================================================================

std::shared_ptr<BluetoothHostInterface> bt_classic_create_host(uint16_t vid, uint16_t pid,
                                                                uint16_t version,
                                                                uint16_t device_id,
                                                                HID_ReportInfo_t *info,
                                                                SubType known_subtype,
                                                                bool known_ready,
                                                                const char *dev_name,
                                                                BtControllerType known_controller_type)
{
    std::shared_ptr<BluetoothHostInterface> host = nullptr;

    // DS3 / DualShock 3 & Navigation Controller
    if (vid == SONY_VID && (pid == SONY_DS3_PID || pid == SONY_PS3_NAV_PID))
    {
        if (info) USB_FreeReportInfo(info);
        host = std::make_shared<BtDs3Host>(device_id);
    }
    // DS4 instruments
    else if ((vid == MADCATZ_VID && (pid == PS4_STRAT_PID || pid == PS4_MADCATZ_DRUM_PID)) ||
             (vid == PDP_VID && pid == PS4_JAG_PID))
    {
        SubType sub = (vid == MADCATZ_VID && pid == PS4_MADCATZ_DRUM_PID) ? RockBandDrums : RockBandGuitar;
        if (info) USB_FreeReportInfo(info);
        host = std::make_shared<BtDs4Host>(device_id, sub,
                                           true, false, false, false, false,
                                           vid, pid);
    }
    // DS4 / DualShock 4 gamepads (first-party or clones/generic DS4)
    else if ((vid == SONY_VID && (pid == SONY_DS4_PID_1 || pid == SONY_DS4_PID_2 || pid == SONY_DS4_PID_3)) ||
             (info && info->foundPS4Usage) ||
             known_controller_type == BtControllerType_BtControllerTypePS4 ||
             (known_subtype != SubType_Unknown && (vid == SONY_VID || (info && info->foundPS4Usage))))
    {
        SubType sub = (known_subtype != SubType_Unknown) ? known_subtype : SubType_Gamepad;
        if (info) USB_FreeReportInfo(info);
        host = std::make_shared<BtDs4Host>(device_id, sub,
                                           false, true, true, true, true,
                                           vid, pid);
    }
    // DS5 / DualSense gamepads
    else if ((vid == SONY_VID && (pid == SONY_DS5_PID || pid == SONY_DS5_EDGE_PID)) ||
             (info && info->foundPS5Usage) || known_controller_type == BtControllerType_BtControllerTypePS5)
    {
        SubType sub = (known_subtype != SubType_Unknown) ? known_subtype : SubType_Gamepad;
        if (info) USB_FreeReportInfo(info);
        host = std::make_shared<BtDs5Host>(device_id, sub,
                                           false, true, true, true, true,
                                           vid, pid);
    }
    // Switch Pro Controller & Joy-Cons & Switch 2 (matched by VID/PID or name for clones)
    else if ((vid == NINTENDO_VID && (pid == SWITCH_PRO_PID || pid == SWITCH_JOYCON_L_PID ||
                                      pid == SWITCH_JOYCON_R_PID || pid == SWITCH_CHARGING_GRIP_PID ||
                                      pid == SWITCH_ONLINE_NES_PID || pid == SWITCH_ONLINE_SNES_PID ||
                                      pid == SWITCH_ONLINE_N64_PID || pid == SWITCH_ONLINE_SEGA_PID ||
                                      (pid >= 0x2000 && pid <= 0x20FF))) ||
             pid == SWITCH_PRO_PID ||
             is_switch_name(dev_name) ||
             known_controller_type == BtControllerType_BtControllerTypeSwitch)
    {
        if (info) USB_FreeReportInfo(info);
        bool is_switch2 = (vid == NINTENDO_VID && (pid == SWITCH_2_PRO_PID || pid == SWITCH_2_JOY_L_PID ||
                                                  pid == SWITCH_2_JOY_R_PID || pid == SWITCH_2_GC_PID)) ||
                          (dev_name && strstr(dev_name, "Switch 2") != nullptr);
        host = std::make_shared<BtSwitchHost>(device_id, is_switch2);
    }
    // HORI Wireless HORIPAD for Steam or Valve devices
    else if ((vid == HORI_VID && pid == HORI_STEAM_CONTROLLER_PID) ||
             vid == VALVE_USB_VID)
    {
        auto generic_host = std::make_shared<BtGenericHost>(device_id, info);
        generic_host->m_subtype = SubType_Gamepad;
        host = generic_host;
    }
    // Nintendo Wii Remote
    else if (vid == NINTENDO_VID && pid == WII_REMOTE_PID)
    {
        if (info) USB_FreeReportInfo(info);
        host = std::make_shared<BtWiiHost>(device_id, false);
    }
    // Nintendo Wii U Pro Controller
    else if (vid == NINTENDO_VID && pid == WII_U_PRO_PID)
    {
        if (info) USB_FreeReportInfo(info);
        host = std::make_shared<BtWiiHost>(device_id, true);
    }
    // Xbox Wireless Controllers, plain HID over Bluetooth
    else if (is_xbox_hid_controller(vid, pid) || known_controller_type == BtControllerType_BtControllerTypeXboxOne)
    {
        host = std::make_shared<BtXboxHost>(device_id, pid, info);
    }
    else
    {
        // Generic HID fallback
        auto generic_host = std::make_shared<BtGenericHost>(device_id, info);
        if (known_subtype != SubType_Unknown)
        {
            generic_host->m_subtype = known_subtype;
        }
        host = generic_host;
    }

    if (host)
    {
        if (!vid && !pid && host->controller_type() == BtControllerType_BtControllerTypeSwitch)
        {
            host->m_vid = NINTENDO_VID;
            host->m_pid = switch_pid_from_name(dev_name);
        }
        else
        {
            host->m_vid = vid;
            host->m_pid = pid;
        }
    }
    return host;
}
