#include "input/fixed.hpp"

FixedInput::FixedInput(proto_FixedInput input) : m_input(input)
{
    setup();
}
void FixedInput::setup(){}
bool FixedInput::tickDigital()
{
    return m_input.value > 0;
}
uint16_t FixedInput::tickAnalog()
{
    return m_input.value;
}