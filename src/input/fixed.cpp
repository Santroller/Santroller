#include "input/fixed.hpp"

FixedInput::FixedInput(proto_FixedInput input) : m_input(input)
{
    setup();
}
void FixedInput::setup() {}
bool FixedInput::tick_digital_raw()
{
    return m_input.value > 0;
}
uint16_t FixedInput::tick_analog()
{
    return m_input.value;
}