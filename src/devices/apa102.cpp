#include "devices/apa102.hpp"
#include "events.pb.h"
#include "main.hpp"
APA102Device::APA102Device(proto_APA102Device device, uint16_t id) : LedDevice(id)
{
}

void APA102Device::update(bool full_poll)
{
}