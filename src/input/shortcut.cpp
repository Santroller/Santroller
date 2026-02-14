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
bool ShortcutInput::tickDigital()
{
    for (auto &input : inputs)
    {
        if (!input->tickDigital())
        {
            return false;
        }
    }
    return true;
}
uint16_t ShortcutInput::tickAnalog()
{
    return tickDigital() ? 65535 : 0;
}