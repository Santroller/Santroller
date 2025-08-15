#include "devices/crkd.hpp"
#include "events.pb.h"
#include "main.hpp"
CrkdDevice::CrkdDevice(proto_CrkdNeckDevice device, uint16_t id) : Device(id), neck(device.uart.block, device.uart.tx, device.uart.rx, device.uart.baudrate)
{
}

void CrkdDevice::update(bool resend_events)
{
    neck.tick();
}