#include "mappings/project_diva_mappings.hpp"
#include "mappings/mapping.hpp"
#include "mappings/switch_arcade_mapping.hpp"
#include "protocols/pdloader.hpp"
#include "protocols/switch_arcade.hpp"

ProjectDivaButtonMapping::ProjectDivaButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
}

void ProjectDivaButtonMapping::update_switch(uint8_t *buf)
{
    auto &report = *reinterpret_cast<SwitchArcadeReport *>(buf);
    switch_arcade_update_button(m_mapping.mapping, m_last_value, report);
}

void ProjectDivaButtonMapping::update_pdloader(uint8_t *buf)
{
    if (!m_last_value || m_mapping.mapping.which_mapping != proto_Output_divaTouch_tag)
        return;
    uint32_t electrode = m_mapping.mapping.mapping.divaTouch;
    if (electrode < 1 || electrode > 32)
        return;
    auto &report = *reinterpret_cast<PDLoaderInputReport *>(buf);
    report.set_slider_touches(report.slider_touches() | (uint32_t(1) << (electrode - 1)));
}

ProjectDivaAxisMapping::ProjectDivaAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : AxisMapping(mapping, std::move(input), id, profile, false)
{
}

void ProjectDivaAxisMapping::update_hid(uint8_t *buf)
{
    return update_xinput(buf);
}

void ProjectDivaAxisMapping::update_wii(uint8_t format, uint8_t *buf)
{
}

void ProjectDivaAxisMapping::update_switch(uint8_t *buf)
{
    auto &report = *reinterpret_cast<SwitchArcadeReport *>(buf);
    switch_arcade_update_axis(m_mapping.mapping, m_calibrated_value, m_centered, report);
    if (m_mapping.mapping.which_mapping != proto_Output_divaAxis_tag)
        return;
    switch (m_mapping.mapping.mapping.divaAxis)
    {
    case ProjectDiva_Slider:
        break;
    default:
        break;
    }
}

void ProjectDivaAxisMapping::update_ps2(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    (void)buf;
    switch (m_mapping.mapping.mapping.divaAxis)
    {
    case ProjectDiva_Slider:
        break;
    default:
        break;
    }
}

void ProjectDivaAxisMapping::update_ps3(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    (void)buf;
    switch (m_mapping.mapping.mapping.divaAxis)
    {
    case ProjectDiva_Slider:
        break;
    default:
        break;
    }
}

void ProjectDivaAxisMapping::update_ps4(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    (void)buf;
    switch (m_mapping.mapping.mapping.divaAxis)
    {
    case ProjectDiva_Slider:
        break;
    default:
        break;
    }
}

void ProjectDivaAxisMapping::update_ps5(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    (void)buf;
    switch (m_mapping.mapping.mapping.divaAxis)
    {
    case ProjectDiva_Slider:
        break;
    default:
        break;
    }
}

void ProjectDivaAxisMapping::update_xinput(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }

    (void)buf;
    switch (m_mapping.mapping.mapping.divaAxis)
    {
    case ProjectDiva_Slider:
        break;
    default:
        break;
    }
}

void ProjectDivaAxisMapping::update_pdloader(uint8_t *buf)
{
    if (m_centered ||
        m_mapping.mapping.mapping.divaAxis != ProjectDiva_Slider)
        return;

    auto &report = *reinterpret_cast<PDLoaderInputReport *>(buf);
    uint32_t position = (m_calibrated_value > UINT16_MAX ? UINT16_MAX : m_calibrated_value) / 2048;
    report.set_slider_touches(report.slider_touches() | (uint32_t(1) << position));
}

void ProjectDivaAxisMapping::update_ogxbox(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    (void)buf;
    switch (m_mapping.mapping.mapping.divaAxis)
    {
    case ProjectDiva_Slider:
        break;
    default:
        break;
    }
}

void ProjectDivaAxisMapping::update_xboxone(uint8_t *buf)
{
}
