#pragma once
#include "devices/bt/bt_host.hpp"
#include "devices/bt/host/generic_host.hpp"
#include "devices/bt/host/xbox_host.hpp"
#include "devices/bt/host/ghl_ios_host.hpp"
#include "devices/bt/host/midi_host.hpp"
#include "devices/bt/host/santroller_host.hpp"
#include "devices/bt/host/steam_host.hpp"
#include "devices/bt/host/switch_host.hpp"
#include "hidparser.h"

// ---------------------------------------------------------------------------
// Factory — given VID/PID from DIS PnP characteristic, create the right host.
// Called by ble_rx after GATTSERVICE_SUBEVENT_DEVICE_INFORMATION_PNP_ID.
// ---------------------------------------------------------------------------
std::shared_ptr<BluetoothHostInterface> ble_create_host(uint16_t vid, uint16_t pid,
                                                         uint16_t version,
                                                         uint16_t device_id,
                                                         HID_ReportInfo_t *info,
                                                         SubType known_subtype = SubType_Unknown);
