#pragma once
#include "devices/bt/bt_host.hpp"
#include "devices/bt/host/ds3_host.hpp"
#include "devices/bt/host/ds4_host.hpp"
#include "devices/bt/host/ds5_host.hpp"
#include "devices/bt/host/switch_host.hpp"
#include "devices/bt/host/xbox_host.hpp"
#include "devices/bt/host/generic_host.hpp"
#include "devices/bt/host/wii_host.hpp"
#include "hidparser.h"

// ---------------------------------------------------------------------------
// Factory — given VID/PID/subtype info, create the right host object.
// Called by bt_classic_rx after SDP query completes.
// ---------------------------------------------------------------------------
std::shared_ptr<BluetoothHostInterface> bt_classic_create_host(uint16_t vid, uint16_t pid,
                                                                uint16_t version,
                                                                uint16_t device_id,
                                                                HID_ReportInfo_t *info,
                                                                SubType known_subtype = SubType_Unknown,
                                                                bool known_ready = false,
                                                                const char *dev_name = nullptr,
                                                                BtControllerType known_controller_type = BtControllerType_BtControllerTypeGeneric);
