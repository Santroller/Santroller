#include "input/shortcut.hpp"

ShortcutInput::ShortcutInput(proto_ShortcutInput input) : m_input(input)
{
    setup();
}
void ShortcutInput::setup(){}
bool ShortcutInput::tickDigital()
{
    for (auto& input: inputs) {
        if (!input->tickDigital()) {
            return false;
        }
    }
    return true;
}
uint16_t ShortcutInput::tickAnalog()
{
    return tickDigital() ? 65535 : 0;
}