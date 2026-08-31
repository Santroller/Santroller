#include "mappings/project_diva_mappings.hpp"
#include "mappings/mapping.hpp"

ProjectDivaButtonMapping::ProjectDivaButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
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
