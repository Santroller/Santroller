#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"

RockBandDrumsButtonMapping::RockBandDrumsButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
}

void RockBandDrumsButtonMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
}
void RockBandDrumsButtonMapping::update_wii(uint8_t format, uint8_t *buf)
{
    // not a thing
}
void RockBandDrumsButtonMapping::update_switch(uint8_t *buf)
{
    // not a thing
}

void RockBandDrumsButtonMapping::update_ps2(uint8_t *buf)
{
    // Not a thing - drums were always usb here
}

void RockBandDrumsButtonMapping::update_ps3(uint8_t *buf)
{
    PS3RockBandDrums_Data_t *report = (PS3RockBandDrums_Data_t *)buf;
    switch (m_mapping.mapping.rbDrumButton)
    {
    case RockBandDrums_A:
        report->a |= m_lastValue;
        break;
    case RockBandDrums_B:
        report->b |= m_lastValue;
        break;
    case RockBandDrums_X:
        report->x |= m_lastValue;
        break;
    case RockBandDrums_Y:
        report->y |= m_lastValue;
        break;
    case RockBandDrums_LeftShoulder:
    case RockBandDrums_Kick1Pedal:
        report->kick1 |= m_lastValue;
        break;
    case RockBandDrums_RightShoulder:
    case RockBandDrums_Kick2Pedal:
        report->kick2 |= m_lastValue;
        break;
    case RockBandDrums_Back:
        report->back |= m_lastValue;
        break;
    case RockBandDrums_Start:
        report->start |= m_lastValue;
        break;
    case RockBandDrums_Guide:
        report->guide |= m_lastValue;
        break;
    case RockBandDrums_Capture:
        report->capture |= m_lastValue;
        break;
    case RockBandDrums_DpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case RockBandDrums_DpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case RockBandDrums_DpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case RockBandDrums_DpadRight:
        report->dpadRight |= m_lastValue;
        break;
    default:
        break;
    }
}

void RockBandDrumsButtonMapping::update_ps4(uint8_t *buf)
{
    PS4RockBandDrums_Data_t *report = (PS4RockBandDrums_Data_t *)buf;
    switch (m_mapping.mapping.rbDrumButton)
    {
    case RockBandDrums_A:
        report->a |= m_lastValue;
        break;
    case RockBandDrums_B:
        report->b |= m_lastValue;
        break;
    case RockBandDrums_X:
        report->x |= m_lastValue;
        break;
    case RockBandDrums_Y:
        report->y |= m_lastValue;
        break;
    case RockBandDrums_LeftShoulder:
    case RockBandDrums_Kick1Pedal:
        report->kick1 |= m_lastValue;
        break;
    case RockBandDrums_RightShoulder:
    case RockBandDrums_Kick2Pedal:
        report->kick2 |= m_lastValue;
        break;
    case RockBandDrums_Back:
        report->back |= m_lastValue;
        break;
    case RockBandDrums_Start:
        report->start |= m_lastValue;
        break;
    case RockBandDrums_Guide:
        report->guide |= m_lastValue;
        break;
    case RockBandDrums_Capture:
        report->capture |= m_lastValue;
        break;
    case RockBandDrums_DpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case RockBandDrums_DpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case RockBandDrums_DpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case RockBandDrums_DpadRight:
        report->dpadRight |= m_lastValue;
        break;
    default:
        break;
    }
}

void RockBandDrumsButtonMapping::update_ps5(uint8_t *buf)
{
    PS5RockBandDrums_Data_t *report = (PS5RockBandDrums_Data_t *)buf;
    switch (m_mapping.mapping.rbDrumButton)
    {
    case RockBandDrums_A:
        report->a |= m_lastValue;
        break;
    case RockBandDrums_B:
        report->b |= m_lastValue;
        break;
    case RockBandDrums_X:
        report->x |= m_lastValue;
        break;
    case RockBandDrums_Y:
        report->y |= m_lastValue;
        break;
    case RockBandDrums_LeftShoulder:
    case RockBandDrums_Kick1Pedal:
        report->kick1 |= m_lastValue;
        break;
    case RockBandDrums_RightShoulder:
    case RockBandDrums_Kick2Pedal:
        report->kick2 |= m_lastValue;
        break;
    case RockBandDrums_Back:
        report->back |= m_lastValue;
        break;
    case RockBandDrums_Start:
        report->start |= m_lastValue;
        break;
    case RockBandDrums_Guide:
        report->guide |= m_lastValue;
        break;
    case RockBandDrums_Capture:
        report->capture |= m_lastValue;
        break;
    case RockBandDrums_DpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case RockBandDrums_DpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case RockBandDrums_DpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case RockBandDrums_DpadRight:
        report->dpadRight |= m_lastValue;
        break;
    default:
        break;
    }
}

void RockBandDrumsButtonMapping::update_xinput(uint8_t *buf)
{
    XInputRockBandDrums_Data_t *report = (XInputRockBandDrums_Data_t *)buf;
    switch (m_mapping.mapping.rbDrumButton)
    {
    case RockBandDrums_A:
        report->a |= m_lastValue;
        break;
    case RockBandDrums_B:
        report->b |= m_lastValue;
        break;
    case RockBandDrums_X:
        report->x |= m_lastValue;
        break;
    case RockBandDrums_Y:
        report->y |= m_lastValue;
        break;
    case RockBandDrums_LeftShoulder:
    case RockBandDrums_Kick1Pedal:
        report->kick1 |= m_lastValue;
        break;
    case RockBandDrums_Kick2Pedal:
        report->kick2 |= m_lastValue;
        break;
    case RockBandDrums_RightShoulder:
        report->cymbalFlag |= m_lastValue;
        break;
    case RockBandDrums_Back:
        report->back |= m_lastValue;
        break;
    case RockBandDrums_Start:
        report->start |= m_lastValue;
        break;
    case RockBandDrums_Guide:
        report->guide |= m_lastValue;
        break;
    case RockBandDrums_Capture:
        report->capture |= m_lastValue;
        break;
    case RockBandDrums_DpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case RockBandDrums_DpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case RockBandDrums_DpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case RockBandDrums_DpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}
void RockBandDrumsButtonMapping::update_ogxbox(uint8_t *buf)
{
    OGXboxRockBandDrums_Data_t *report = (OGXboxRockBandDrums_Data_t *)buf;
    switch (m_mapping.mapping.rbDrumButton)
    {
    case RockBandDrums_A:
        report->a |= m_lastValue;
        break;
    case RockBandDrums_B:
        report->b |= m_lastValue;
        break;
    case RockBandDrums_X:
        report->x |= m_lastValue;
        break;
    case RockBandDrums_Y:
        report->y |= m_lastValue;
        break;
    case RockBandDrums_Kick1Pedal:
        report->kick1 |= m_lastValue;
        break;
    case RockBandDrums_Kick2Pedal:
        report->kick2 |= m_lastValue;
        break;
    case RockBandDrums_Back:
        report->back |= m_lastValue;
        break;
    case RockBandDrums_Start:
        report->start |= m_lastValue;
        break;
    case RockBandDrums_Capture:
        // report->capture |= m_lastValue;
        break;
    case RockBandDrums_DpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case RockBandDrums_DpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case RockBandDrums_DpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case RockBandDrums_DpadRight:
        report->dpadRight |= m_lastValue;
        break;
    default:
        break;
    }
}
void RockBandDrumsButtonMapping::update_xboxone(uint8_t *buf)
{
    
    XboxOneRockBandDrums_Data_t *report = (XboxOneRockBandDrums_Data_t *)buf;
    switch (m_mapping.mapping.rbDrumButton)
    {
    case RockBandDrums_A:
        report->a |= m_lastValue;
        break;
    case RockBandDrums_B:
        report->b |= m_lastValue;
        break;
    case RockBandDrums_X:
        report->x |= m_lastValue;
        break;
    case RockBandDrums_Y:
        report->y |= m_lastValue;
        break;
    case RockBandDrums_LeftShoulder:
    case RockBandDrums_Kick1Pedal:
        report->leftShoulder |= m_lastValue;
        break;
    case RockBandDrums_RightShoulder:
    case RockBandDrums_Kick2Pedal:
        report->rightShoulder |= m_lastValue;
        break;
    case RockBandDrums_Back:
        report->back |= m_lastValue;
        break;
    case RockBandDrums_Start:
        report->start |= m_lastValue;
        break;
    case RockBandDrums_Guide:
        report->guide |= m_lastValue;
        break;
    case RockBandDrums_Capture:
        if (m_lastValue)
        {
            report->consoleFunctions[0] = 0x01;
        }
        break;
    case RockBandDrums_DpadUp:
        report->dpadUp |= m_lastValue;
        break;
    case RockBandDrums_DpadDown:
        report->dpadDown |= m_lastValue;
        break;
    case RockBandDrums_DpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case RockBandDrums_DpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}