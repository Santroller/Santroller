#include "devices/bt/host/stadia_host.hpp"
#include "devices/bt/bluetooth_stack.hpp"
#include "btstack.h"
#include "utils.h"
#include "ble/gatt-service/hids_host.h"

void BleStadiaHost::handle_report(const uint8_t *data, uint16_t len)
{
    BluetoothHostInterface::handle_report(data, len);
    stadia_parse_report(data, len, m_data);
}

bool BleStadiaHost::tick_digital(proto_Output &type)
{
    return stadia_tick_digital(m_data, type);
}

uint16_t BleStadiaHost::tick_analog(proto_Output &type)
{
    return stadia_tick_analog(m_data, type);
}

void BleStadiaHost::update(bool full_poll, bool send_events)
{
    BluetoothHostInterface::update(full_poll, send_events);
    uint32_t now = millis();
    if (!m_cid || !m_rumble.should_send(now))
        return;
    stadia_build_rumble(m_rumble.left, m_rumble.right, m_out_buf);
    uint8_t status;
    {
        BtStackLock lock;
        status = hids_host_send_write_report(m_cid, STADIA_RUMBLE_REPORT_ID, HID_REPORT_TYPE_OUTPUT,
                                             m_out_buf, sizeof(m_out_buf));
    }
    if (status == ERROR_CODE_SUCCESS)
        m_rumble.sent(now);
}
