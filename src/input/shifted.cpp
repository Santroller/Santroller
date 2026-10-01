#include "input/shifted.hpp"

ShiftedInput::ShiftedInput()
{
}

void ShiftedInput::load(proto_ShiftedInput config, std::unique_ptr<Input> input, std::unique_ptr<Input> shift)
{
    m_input = std::move(input);
    m_shift = std::move(shift);
    m_invert_shift = config.has_invertShift ? config.invertShift : false;
}

void ShiftedInput::setup()
{
    if (m_input)
    {
        m_input->setup();
    }
    if (m_shift)
    {
        m_shift->setup();
    }
}

bool ShiftedInput::tick_digital()
{
    if (!m_input || !m_shift)
    {
        return false;
    }
    bool input_pressed = m_input->tick_digital();
    if (!input_pressed)
    {
        return false;
    }
    bool shift_pressed = m_shift->tick_digital();
    return m_invert_shift ? !shift_pressed : shift_pressed;
}

uint16_t ShiftedInput::tick_analog()
{
    if (!m_input || !m_shift)
    {
        return 0;
    }
    bool shift_pressed = m_shift->tick_digital();
    bool condition_met = m_invert_shift ? !shift_pressed : shift_pressed;
    if (!condition_met)
    {
        return 0;
    }
    return m_input->tick_analog();
}
