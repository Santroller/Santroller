#include "devices/bt/bt_ble_host.hpp"
#include "hidparser.h"
#include "btstack.h"
#include "emulation/usb/usb_devices.h"

#include <memory>

// ============================================================================
// Factory
// ============================================================================

std::shared_ptr<BluetoothHostInterface> ble_create_host(uint16_t vid, uint16_t pid,
                                                         uint16_t version,
                                                         uint16_t device_id,
                                                         HID_ReportInfo_t *info,
                                                         SubType known_subtype)
{
    bool is_santroller = (vid == SANTROLLER_VID && (pid == SANTROLLER_PID || pid == SANTROLLER_V1_PID || pid == SANTROLLER_V1_BLE_PID));

    if (is_santroller)
    {
        // info is null when the report map couldn't be read
        const bool is_v2 = info && info->foundSantrollerV2OutputUsage;
        auto host = std::make_shared<BleSantrollerHost>(device_id, is_v2, version, known_subtype);
        host->m_vid = vid ? vid : SANTROLLER_VID;
        host->m_pid = pid ? pid : (is_v2 ? SANTROLLER_PID : SANTROLLER_V1_BLE_PID);
        return host;
    }

    // 2026 Steam Controller (Triton) over HID over GATT
    if (vid == VALVE_USB_VID && pid == VALVE_STEAM_TRITON_BLE_PID)
    {
        auto host = std::make_shared<BleSteamTritonHost>(device_id);
        host->m_vid = vid;
        host->m_pid = pid;
        return host;
    }

    // Valve Steam Controller / HORI Steam Controller
    if (vid == VALVE_USB_VID || (vid == HORI_VID && pid == HORI_STEAM_CONTROLLER_PID))
    {
        auto host = std::make_shared<BleSteamHost>(device_id);
        host->m_vid = vid;
        host->m_pid = pid;
        if (known_subtype != SubType_Unknown)
        {
            host->m_subtype = known_subtype;
        }
        return host;
    }

    // Switch 2 BLE Controllers
    if (vid == NINTENDO_VID && (pid == SWITCH_2_PRO_PID || pid == SWITCH_2_JOY_L_PID ||
                                pid == SWITCH_2_JOY_R_PID || pid == SWITCH_2_GC_PID))
    {
        auto host = std::make_shared<BleSwitch2Host>(device_id);
        host->m_vid = vid;
        host->m_pid = pid;
        return host;
    }

    // Xbox Wireless Controllers, plain HID over GATT with their own button layout
    if (is_xbox_hid_controller(vid, pid))
    {
        auto host = std::make_shared<BleXboxHost>(device_id, pid, info);
        host->m_vid = vid;
        return host;
    }

    // Any other standard HID-over-GATT gamepad uses report parsing via fill_generic_report.
    auto host = std::make_shared<BleGenericHost>(device_id, info);
    if (known_subtype != SubType_Unknown)
    {
        host->m_subtype = known_subtype;
    }
    host->m_vid = vid;
    host->m_pid = pid;
    return host;
}
