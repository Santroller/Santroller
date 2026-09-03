#include "mappings/powergig_mappings.hpp"
#include "protocols/ps3.hpp"

PowerGigGuitarButtonMapping::PowerGigGuitarButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
}

void PowerGigGuitarButtonMapping::update_ps3(uint8_t *buf)
{
    PS3PowerGigGuitar_Data_t *report = (PS3PowerGigGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.rbButton)
    {
    case RockBandGuitar_Green:
        report->a |= m_last_value;
        break;
    case RockBandGuitar_Red:
        report->b |= m_last_value;
        break;
    case RockBandGuitar_Yellow:
        report->y |= m_last_value;
        break;
    case RockBandGuitar_Blue:
        report->x |= m_last_value;
        break;
    case RockBandGuitar_Orange:
        report->leftShoulder |= m_last_value;
        break;
    case RockBandGuitar_Pedal:
        report->tilt |= m_last_value;
        break;
    case RockBandGuitar_SoloGreen:
        report->a |= m_last_value;
        report->solo |= m_last_value;
        break;
    case RockBandGuitar_SoloRed:
        report->b |= m_last_value;
        report->solo |= m_last_value;
        break;
    case RockBandGuitar_SoloYellow:
        report->y |= m_last_value;
        report->solo |= m_last_value;
        break;
    case RockBandGuitar_SoloBlue:
        report->x |= m_last_value;
        report->solo |= m_last_value;
        break;
    case RockBandGuitar_SoloOrange:
        report->leftShoulder |= m_last_value;
        report->solo |= m_last_value;
        break;
    }
}

PowerGigGuitarAxisMapping::PowerGigGuitarAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : AxisMapping(mapping, std::move(input), id, profile, mapping.mapping.mapping.rbAxis == RockBandGuitar_Whammy)
{
}

void PowerGigGuitarAxisMapping::update_ps3(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }

    PS3PowerGigGuitar_Data_t *report = (PS3PowerGigGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.rbAxis)
    {
    case RockBandGuitar_Whammy:
        report->whammy = m_calibrated_value >> 8;
        break;
    case RockBandGuitar_Tilt:
        report->tilt = m_calibrated_value >> 8;
        break;
    case RockBandGuitar_Pickup:
        report->pickup = m_calibrated_value >> 8;
        break;
    }
}

PowerGigDrumsButtonMapping::PowerGigDrumsButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
}

void PowerGigDrumsButtonMapping::update_ps3(uint8_t *buf)
{
    PS3PowerGigDrums_Data_t *report = (PS3PowerGigDrums_Data_t *)buf;
    switch (m_mapping.mapping.mapping.rbDrumButton)
    {
    case RockBandDrums_Kick1Pedal:
    case RockBandDrums_Kick2Pedal:
        report->leftThumbClick |= m_last_value;
        break;
    }
}

PowerGigDrumsAxisMapping::PowerGigDrumsAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : AxisMapping(mapping, std::move(input), id, profile, true)
{
}

void PowerGigDrumsAxisMapping::update_ps3(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }

    PS3PowerGigDrums_Data_t *report = (PS3PowerGigDrums_Data_t *)buf;
    uint8_t pressure = m_calibrated_value >> 8;
    switch (m_mapping.mapping.mapping.rbDrumAxis)
    {
    case RockBandDrums_RedPad:
        report->pressure_red = pressure;
        report->left_red = true;
        break;
    case RockBandDrums_YellowPad:
        report->pressure_yellow = pressure;
        report->left_yellow = true;
        break;
    case RockBandDrums_BluePad:
        report->pressure_blue = pressure;
        report->left_blue = true;
        break;
    case RockBandDrums_GreenPad:
        report->pressure_green = pressure;
        report->left_green = true;
        break;
    default:
        break;
    }
}