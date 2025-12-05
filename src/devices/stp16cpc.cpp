#include "devices/stp16cpc.hpp"
#include "events.pb.h"
#include "main.hpp"
STP16CPCDevice::STP16CPCDevice(proto_STP16CPCDevice device, uint16_t id) : LedDevice(id)
{
}

void STP16CPCDevice::update(bool full_poll)
{
}