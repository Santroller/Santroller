#include "devices/crkd_drum.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "usb/device/hid_device.h"
#include "config.hpp"
CrkdDrumDevice::CrkdDrumDevice(proto_CrkdDrumDevice device, uint16_t id) : Device(id), drum(device.uart.block, device.uart.tx, device.uart.rx, device.uart.baudrate), m_device(device)
{
}

void CrkdDrumDevice::begin()
{
    drum.begin();
}
void CrkdDrumDevice::end(bool full)
{
    drum.end();
}
void CrkdDrumDevice::update(bool full_poll, bool send_events)
{
    drum.tick();
    if (m_lastConnected != drum.is_connected() || full_poll)
    {
        m_lastConnected = drum.is_connected();
        proto_Event event = {which_event : proto_Event_device_tag, event : {device : {m_id, m_lastConnected}}};
        HIDConfigDevice::send_event(event, true);
        event = {
            which_event : proto_Event_crkdDrum_tag,
            event : {
                crkdDrum : {
                    m_id,
                    type : CrkdDrumCalibrationType_Debounce,
                    data : {
                        drum.m_debounceParams.red_pad,
                        drum.m_debounceParams.yellow_pad,
                        drum.m_debounceParams.blue_pad,
                        drum.m_debounceParams.green_pad,
                        drum.m_debounceParams.orange_pad,
                        drum.m_debounceParams.yellow_cymbal,
                        drum.m_debounceParams.blue_cymbal,
                        drum.m_debounceParams.green_cymbal,
                        drum.m_debounceParams.kick1,
                        drum.m_debounceParams.kick2},
                }
            }
        };
        HIDConfigDevice::send_event(event, true);
        event = {
            which_event : proto_Event_crkdDrum_tag,
            event : {
                crkdDrum : {
                    m_id,
                    type : CrkdDrumCalibrationType_Min,
                    data : {
                        drum.m_minParams.red_pad,
                        drum.m_minParams.yellow_pad,
                        drum.m_minParams.blue_pad,
                        drum.m_minParams.green_pad,
                        drum.m_minParams.orange_pad,
                        drum.m_minParams.yellow_cymbal,
                        drum.m_minParams.blue_cymbal,
                        drum.m_minParams.green_cymbal,
                        drum.m_minParams.kick1,
                        drum.m_minParams.kick2},
                }
            }
        };
        HIDConfigDevice::send_event(event, true);
        event = {
            which_event : proto_Event_crkdDrum_tag,
            event : {
                crkdDrum : {
                    m_id,
                    type : CrkdDrumCalibrationType_Max,
                    data : {
                        drum.m_maxParams.red_pad,
                        drum.m_maxParams.yellow_pad,
                        drum.m_maxParams.blue_pad,
                        drum.m_maxParams.green_pad,
                        drum.m_maxParams.orange_pad,
                        drum.m_maxParams.yellow_cymbal,
                        drum.m_maxParams.blue_cymbal,
                        drum.m_maxParams.green_cymbal,
                        drum.m_maxParams.kick1,
                        drum.m_maxParams.kick2},
                }
            }
        };
        HIDConfigDevice::send_event(event, true);
    }
}

bool CrkdDrumDevice::using_pin(uint8_t pin)
{
    return pin == m_device.uart.rx || pin == m_device.uart.tx;
}