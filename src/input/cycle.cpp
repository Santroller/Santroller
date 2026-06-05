#include "input/cycle.hpp"
#include "utils.h"
#include "config.hpp"
#include <pb_encode.h>
#include "usb/device/hid_device.h"
#include "events.pb.h"

CycleInput::CycleInput()
{
}
void CycleInput::load(proto_CycleInput config, std::shared_ptr<CycleDevice> device, std::unique_ptr<Input> input) {
    m_input = std::move(input);
    m_device = device;
    m_last_toggled = 0;
}
void CycleInput::setup()
{
    m_input->setup();
}
bool CycleInput::tickDigital()
{
    return tickAnalog() > 0;
}
uint16_t CycleInput::tickAnalog()
{
    if (m_input->tickDigital() && (millis() - m_last_toggled > 500)) {
        m_device->cycle();
        m_last_toggled = millis();
    }
    return m_device->get_value();
}