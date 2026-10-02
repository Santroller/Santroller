#include "input/shortcut.hpp"

ShortcutInput::ShortcutInput()
{
}
void ShortcutInput::setup()
{
    for (auto &input : inputs)
    {
        input->setup();
    }
}
bool ShortcutInput::tick_digital()
{
    if (inputs.empty())
    {
        return false;
    }
    for (auto &input : inputs)
    {
        if (!input->tick_digital())
        {
            return false;
        }
    }
    return true;
}
uint16_t ShortcutInput::tick_analog()
{
    return tick_digital() ? 65535 : 0;
}