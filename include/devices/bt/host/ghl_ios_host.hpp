#pragma once
#include "devices/bt/bt_host.hpp"

// ---------------------------------------------------------------------------
// Guitar Hero Live iOS BLE Guitar
// Direct GATT characteristic notifications (UUID: 533e1524-3abe-f33f-cd00-594e8b0a8ea3)
// ---------------------------------------------------------------------------
class BleGhlIosHost : public BluetoothHostInterface
{
public:
    BleGhlIosHost(uint16_t id) : BluetoothHostInterface(id)
    {
        m_subtype = LiveGuitar;
    }
    ~BleGhlIosHost() {}

    BtControllerType controller_type() const override { return BtControllerType_BtControllerTypeGhlIos; }

    bool tick_digital(proto_Output &type) override;
    uint16_t tick_analog(proto_Output &type) override;
};
