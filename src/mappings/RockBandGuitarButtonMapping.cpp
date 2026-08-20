#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"

RockBandGuitarButtonMapping::RockBandGuitarButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
    
}

void RockBandGuitarButtonMapping::update_hid(uint8_t *buf)
{
    // santroller hid uses an xinput style report descriptor for compatibility reasons
    return update_xinput(buf);
}
void RockBandGuitarButtonMapping::update_wii(uint8_t format, uint8_t *buf)
{
   // not a thing, was hid
}
void RockBandGuitarButtonMapping::update_switch(uint8_t *buf)
{
    SwitchFestivalProGuitarLayer_Data_t *report = (SwitchFestivalProGuitarLayer_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghButton)
    {
    case RockBandGuitar_Green:
        report->a |= m_lastValue;
        break;
    case RockBandGuitar_Red:
        report->b |= m_lastValue;
        break;
    case RockBandGuitar_Yellow:
        report->y |= m_lastValue;
        break;
    case RockBandGuitar_Blue:
        report->x |= m_lastValue;
        break;
    case RockBandGuitar_Orange:
        report->leftShoulder |= m_lastValue;
        break;
    case RockBandGuitar_Pedal:
        report->rightShoulder |= m_lastValue;
        break;
    case RockBandGuitar_SoloGreen:
        report->a |= m_lastValue;
        break;
    case RockBandGuitar_SoloRed:
        report->b |= m_lastValue;
        break;
    case RockBandGuitar_SoloYellow:
        report->y |= m_lastValue;
        break;
    case RockBandGuitar_SoloBlue:
        report->x |= m_lastValue;
        break;
    case RockBandGuitar_SoloOrange:
        report->leftShoulder |= m_lastValue;
        break;
    case RockBandGuitar_Back:
        report->back |= m_lastValue;
        break;
    case RockBandGuitar_Start:
        report->start |= m_lastValue;
        break;
    case RockBandGuitar_Guide:
        report->guide |= m_lastValue;
        break;
    case RockBandGuitar_Capture:
        report->capture |= m_lastValue;
        break;
    case RockBandGuitar_StrumUp:
        report->dpadUp |= m_lastValue;
        break;
    case RockBandGuitar_StrumDown:
        report->dpadDown |= m_lastValue;
        break;
    case RockBandGuitar_DpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case RockBandGuitar_DpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}

void RockBandGuitarButtonMapping::update_ps2(uint8_t *buf)
{
    // was hid
}

void RockBandGuitarButtonMapping::update_ps3(uint8_t *buf)
{
    PS3RockBandGuitar_Data_t *report = (PS3RockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghButton)
    {
    case RockBandGuitar_Green:
        report->a |= m_lastValue;
        break;
    case RockBandGuitar_Red:
        report->b |= m_lastValue;
        break;
    case RockBandGuitar_Yellow:
        report->y |= m_lastValue;
        break;
    case RockBandGuitar_Blue:
        report->x |= m_lastValue;
        break;
    case RockBandGuitar_Orange:
        report->leftShoulder |= m_lastValue;
        break;
    case RockBandGuitar_Pedal:
        report->tilt |= m_lastValue;
        break;
    case RockBandGuitar_SoloGreen:
        report->a |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitar_SoloRed:
        report->b |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitar_SoloYellow:
        report->y |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitar_SoloBlue:
        report->x |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitar_SoloOrange:
        report->leftShoulder |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitar_Back:
        report->back |= m_lastValue;
        break;
    case RockBandGuitar_Start:
        report->start |= m_lastValue;
        break;
    case RockBandGuitar_Guide:
        report->guide |= m_lastValue;
        break;
    case RockBandGuitar_Capture:
        report->capture |= m_lastValue;
        break;
    case RockBandGuitar_StrumUp:
        report->dpadUp |= m_lastValue;
        break;
    case RockBandGuitar_StrumDown:
        report->dpadDown |= m_lastValue;
        break;
    case RockBandGuitar_DpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case RockBandGuitar_DpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}

void RockBandGuitarButtonMapping::update_ps4(uint8_t *buf)
{
    PS4RockBandGuitar_Data_t *report = (PS4RockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghButton)
    {
    case RockBandGuitar_Green:
        report->a |= m_lastValue;
        break;
    case RockBandGuitar_Red:
        report->b |= m_lastValue;
        break;
    case RockBandGuitar_Yellow:
        report->y |= m_lastValue;
        break;
    case RockBandGuitar_Blue:
        report->x |= m_lastValue;
        break;
    case RockBandGuitar_Orange:
        report->leftShoulder |= m_lastValue;
        break;
    case RockBandGuitar_Pedal:
        report->rightShoulder |= m_lastValue;
        break;
    case RockBandGuitar_SoloGreen:
        report->a |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitar_SoloRed:
        report->b |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitar_SoloYellow:
        report->y |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitar_SoloBlue:
        report->x |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitar_SoloOrange:
        report->leftShoulder |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitar_Back:
        report->back |= m_lastValue;
        break;
    case RockBandGuitar_Start:
        report->start |= m_lastValue;
        break;
    case RockBandGuitar_Guide:
        report->guide |= m_lastValue;
        break;
    case RockBandGuitar_Capture:
        report->capture |= m_lastValue;
        break;
    case RockBandGuitar_StrumUp:
        report->dpadUp |= m_lastValue;
        break;
    case RockBandGuitar_StrumDown:
        report->dpadDown |= m_lastValue;
        break;
    case RockBandGuitar_DpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case RockBandGuitar_DpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}

void RockBandGuitarButtonMapping::update_ps5(uint8_t *buf)
{
    PS5RockBandGuitar_Data_t *report = (PS5RockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghButton)
    {
    case RockBandGuitar_Green:
        report->a |= m_lastValue;
        break;
    case RockBandGuitar_Red:
        report->b |= m_lastValue;
        break;
    case RockBandGuitar_Yellow:
        report->y |= m_lastValue;
        break;
    case RockBandGuitar_Blue:
        report->x |= m_lastValue;
        break;
    case RockBandGuitar_Orange:
        report->leftShoulder |= m_lastValue;
        break;
    case RockBandGuitar_Pedal:
        report->rightShoulder |= m_lastValue;
        break;
    case RockBandGuitar_SoloGreen:
        report->a |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitar_SoloRed:
        report->b |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitar_SoloYellow:
        report->y |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitar_SoloBlue:
        report->x |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitar_SoloOrange:
        report->leftShoulder |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitar_Back:
        report->back |= m_lastValue;
        break;
    case RockBandGuitar_Start:
        report->start |= m_lastValue;
        break;
    case RockBandGuitar_Guide:
        report->guide |= m_lastValue;
        break;
    case RockBandGuitar_Capture:
        report->capture |= m_lastValue;
        break;
    case RockBandGuitar_StrumUp:
        report->dpadUp |= m_lastValue;
        break;
    case RockBandGuitar_StrumDown:
        report->dpadDown |= m_lastValue;
        break;
    case RockBandGuitar_DpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case RockBandGuitar_DpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}

void RockBandGuitarButtonMapping::update_xinput(uint8_t *buf)
{
    XInputRockBandGuitar_Data_t *report = (XInputRockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghButton)
    {
    case RockBandGuitar_Green:
        report->a |= m_lastValue;
        break;
    case RockBandGuitar_Red:
        report->b |= m_lastValue;
        break;
    case RockBandGuitar_Yellow:
        report->y |= m_lastValue;
        break;
    case RockBandGuitar_Blue:
        report->x |= m_lastValue;
        break;
    case RockBandGuitar_Orange:
        report->leftShoulder |= m_lastValue;
        break;
    case RockBandGuitar_Pedal:
        report->rightShoulder |= m_lastValue;
        break;
    case RockBandGuitar_SoloGreen:
        report->a |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitar_SoloRed:
        report->b |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitar_SoloYellow:
        report->y |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitar_SoloBlue:
        report->x |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitar_SoloOrange:
        report->leftShoulder |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitar_Back:
        report->back |= m_lastValue;
        break;
    case RockBandGuitar_Start:
        report->start |= m_lastValue;
        break;
    case RockBandGuitar_Guide:
        report->guide |= m_lastValue;
        break;
    case RockBandGuitar_Capture:
        report->capture |= m_lastValue;
        break;
    case RockBandGuitar_StrumUp:
        report->dpadUp |= m_lastValue;
        break;
    case RockBandGuitar_StrumDown:
        report->dpadDown |= m_lastValue;
        break;
    case RockBandGuitar_DpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case RockBandGuitar_DpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}
void RockBandGuitarButtonMapping::update_ogxbox(uint8_t *buf)
{
    OGXboxRockBandGuitar_Data_t *report = (OGXboxRockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghButton)
    {
    case RockBandGuitar_Green:
        report->a |= m_lastValue;
        break;
    case RockBandGuitar_Red:
        report->b |= m_lastValue;
        break;
    case RockBandGuitar_Yellow:
        report->y |= m_lastValue;
        break;
    case RockBandGuitar_Blue:
        report->x |= m_lastValue;
        break;
    case RockBandGuitar_Orange:
        report->leftShoulder |= m_lastValue;
        break;
    case RockBandGuitar_Pedal:
        report->rightShoulder |= m_lastValue;
        break;
    case RockBandGuitar_SoloGreen:
        report->a |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitar_SoloRed:
        report->b |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitar_SoloYellow:
        report->y |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitar_SoloBlue:
        report->x |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitar_SoloOrange:
        report->leftShoulder |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitar_Back:
        report->back |= m_lastValue;
        break;
    case RockBandGuitar_Start:
        report->start |= m_lastValue;
        break;
    case RockBandGuitar_Guide:
        break;
    case RockBandGuitar_Capture:
        // report->capture |= m_lastValue;
        break;
    case RockBandGuitar_StrumUp:
        report->dpadUp |= m_lastValue;
        break;
    case RockBandGuitar_StrumDown:
        report->dpadDown |= m_lastValue;
        break;
    case RockBandGuitar_DpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case RockBandGuitar_DpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}
void RockBandGuitarButtonMapping::update_xboxone(uint8_t *buf)
{
    XboxOneRockBandGuitar_Data_t *report = (XboxOneRockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghButton)
    {
    case RockBandGuitar_Green:
        report->a |= m_lastValue;
        break;
    case RockBandGuitar_Red:
        report->b |= m_lastValue;
        break;
    case RockBandGuitar_Yellow:
        report->y |= m_lastValue;
        break;
    case RockBandGuitar_Blue:
        report->x |= m_lastValue;
        break;
    case RockBandGuitar_Orange:
        report->leftShoulder |= m_lastValue;
        break;
    case RockBandGuitar_Pedal:
        report->rightShoulder |= m_lastValue;
        break;
    case RockBandGuitar_SoloGreen:
        report->a |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitar_SoloRed:
        report->b |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitar_SoloYellow:
        report->y |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitar_SoloBlue:
        report->x |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitar_SoloOrange:
        report->leftShoulder |= m_lastValue;
        report->solo |= m_lastValue;
        break;
    case RockBandGuitar_Back:
        report->back |= m_lastValue;
        break;
    case RockBandGuitar_Start:
        report->start |= m_lastValue;
        break;
    case RockBandGuitar_Guide:
        report->guide |= m_lastValue;
        break;
    case RockBandGuitar_Capture:
        if (m_lastValue)
        {
            report->consoleFunctions[0] = 0x01;
        }
        break;
    case RockBandGuitar_StrumUp:
        report->dpadUp |= m_lastValue;
        break;
    case RockBandGuitar_StrumDown:
        report->dpadDown |= m_lastValue;
        break;
    case RockBandGuitar_DpadLeft:
        report->dpadLeft |= m_lastValue;
        break;
    case RockBandGuitar_DpadRight:
        report->dpadRight |= m_lastValue;
        break;
    }
}