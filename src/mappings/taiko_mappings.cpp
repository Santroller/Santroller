#include "mappings/taiko_mappings.hpp"
#include "mappings/switch_arcade_mapping.hpp"
#include "protocols/ps3.hpp"
#include "protocols/ps2.hpp"
#include "protocols/switch_arcade.hpp"
#include "protocols/wii.hpp"

TaikoButtonMapping::TaikoButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : PS3GamepadButtonMapping(mapping, std::move(input), id, profile)
{
}

void TaikoButtonMapping::update_switch(uint8_t *buf)
{
    auto &report = *reinterpret_cast<SwitchArcadeReport *>(buf);
    switch_arcade_update_button(m_mapping.mapping, m_last_value, report);
}

void TaikoButtonMapping::update_wii(uint8_t, uint8_t *buf)
{
    if (!m_last_value || m_mapping.mapping.which_mapping != proto_Output_gamepadButton_tag)
        return;

    auto &report = *reinterpret_cast<WiiTaikoData_t *>(buf);
    switch (m_mapping.mapping.mapping.gamepadButton)
    {
    case Gamepad_DpadLeft: report.buttons |= 1 << 5; break;
    case Gamepad_DpadDown: report.buttons |= 1 << 6; break;
    case Gamepad_A: report.buttons |= 1 << 4; break;
    case Gamepad_B: report.buttons |= 1 << 3; break;
    default: break;
    }
}

TaikoAxisMapping::TaikoAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : PS3GamepadAxisMapping(mapping, std::move(input), id, profile)
{
}

void TaikoAxisMapping::update_switch(uint8_t *buf)
{
    auto &report = *reinterpret_cast<SwitchArcadeReport *>(buf);
    switch_arcade_update_axis(m_mapping.mapping, m_calibrated_value, m_centered, report);
}

void TaikoAxisMapping::update_ps3(uint8_t *buf)
{
    PS3GamepadAxisMapping::update_ps3(buf);
    if (m_centered)
        return;

    auto &report = *reinterpret_cast<PS3Gamepad_Data_t *>(buf);
    switch (m_mapping.mapping.mapping.gamepadAxis)
    {
    case Gamepad_LeftTrigger:
        report.l2 = m_calibrated_value > 60000;
        break;
    case Gamepad_RightTrigger:
        report.r2 = m_calibrated_value > 60000;
        break;
    default:
        break;
    }
}

void TaikoAxisMapping::update_ps2(uint8_t *buf)
{
    GamepadAxisMapping::update_ps2(buf);
    if (m_centered)
        return;

    auto &report = *reinterpret_cast<PS2Gamepad_Data_t *>(buf);
    switch (m_mapping.mapping.mapping.gamepadAxis)
    {
    case Gamepad_LeftTrigger:
        report.l2 = m_calibrated_value > 60000;
        break;
    case Gamepad_RightTrigger:
        report.r2 = m_calibrated_value > 60000;
        break;
    default:
        break;
    }
}

void TaikoAxisMapping::update_wii(uint8_t, uint8_t *buf)
{
    if (m_centered)
        return;

    auto &report = *reinterpret_cast<WiiTaikoData_t *>(buf);
    switch (m_mapping.mapping.mapping.gamepadAxis)
    {
    case Gamepad_LeftTrigger:
        if (m_calibrated_value > 60000) report.buttons |= 1 << 5;
        break;
    case Gamepad_RightTrigger:
        if (m_calibrated_value > 60000) report.buttons |= 1 << 3;
        break;
    default:
        break;
    }
}
