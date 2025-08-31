#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"

GuitarHeroGuitarButtonMapping::GuitarHeroGuitarButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id) : Mapping(id), m_mapping(mapping), m_input(std::move(input))
{
}

// deal with debounce and all the other fun things
void GuitarHeroGuitarButtonMapping::update(bool full_poll)
{
    auto val = m_input->tickDigital();
    if (val != m_lastValue || full_poll)
    {
        proto_Event event = {which_event : proto_Event_button_tag, event : {button : {m_id, val, val}}};
        send_event(event);
        m_lastValue = val;
        // debouce would just be storing the millis() from the last poll and simply using that as the indication for if the input is active instead of lastValue
    }
}
void GuitarHeroGuitarButtonMapping::update_hid(uint8_t *buf)
{
    PCGuitarHeroGuitar_Data_t *report = (PCGuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghButton)
    {
    case GuitarHeroGuitarGreen:
        report->a = m_lastValue;
        break;
    case GuitarHeroGuitarRed:
        report->b = m_lastValue;
        break;
    case GuitarHeroGuitarYellow:
        report->y = m_lastValue;
        break;
    case GuitarHeroGuitarBlue:
        report->x = m_lastValue;
        break;
    case GuitarHeroGuitarOrange:
        report->leftShoulder = m_lastValue;
        break;
    case GuitarHeroGuitarTapGreen:
        report->tapGreen = m_lastValue;
        break;
    case GuitarHeroGuitarTapRed:
        report->tapRed = m_lastValue;
        break;
    case GuitarHeroGuitarTapYellow:
        report->tapYellow = m_lastValue;
        break;
    case GuitarHeroGuitarTapBlue:
        report->tapBlue = m_lastValue;
        break;
    case GuitarHeroGuitarTapOrange:
        report->tapOrange = m_lastValue;
        break;
    case GuitarHeroGuitarBack:
        report->back = m_lastValue;
        break;
    case GuitarHeroGuitarStart:
        report->start = m_lastValue;
        break;
    case GuitarHeroGuitarGuide:
        report->guide = m_lastValue;
        break;
    case GuitarHeroGuitarStrumUp:
        report->dpadUp = m_lastValue;
        break;
    case GuitarHeroGuitarStrumDown:
        report->dpadDown = m_lastValue;
        break;
    case GuitarHeroGuitarDpadLeft:
        report->dpadLeft = m_lastValue;
        break;
    case GuitarHeroGuitarDpadRight:
        report->dpadRight = m_lastValue;
        break;
    }
}
void GuitarHeroGuitarButtonMapping::update_wii(uint8_t *buf)
{
    // TODO: we have to deal with data formats probably
    WiiGuitarDataFormat3_t *report = (WiiGuitarDataFormat3_t *)buf;
    switch (m_mapping.mapping.ghAxis)
    {
    case GuitarHeroGuitarLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarLeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarWhammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarTilt:
        // report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}
void GuitarHeroGuitarButtonMapping::update_switch(uint8_t *buf)
{
    SwitchFestivalProGuitarLayer_Data_t *report = (SwitchFestivalProGuitarLayer_Data_t *)buf;
    switch (m_mapping.mapping.ghAxis)
    {
    case GuitarHeroGuitarLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarLeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarWhammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarTilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}

void GuitarHeroGuitarButtonMapping::update_ps2(uint8_t *buf)
{
    PS2GuitarHeroGuitar_Data_t *report = (PS2GuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghAxis)
    {
    case GuitarHeroGuitarLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarLeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarWhammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarTilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}

void GuitarHeroGuitarButtonMapping::update_ps3(uint8_t *buf)
{
    PS3GuitarHeroGuitar_Data_t *report = (PS3GuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghAxis)
    {
    case GuitarHeroGuitarLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarLeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarWhammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarTilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    }
}

void GuitarHeroGuitarButtonMapping::update_ps4(uint8_t *buf)
{
    PS4RockBandGuitar_Data_t *report = (PS4RockBandGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghAxis)
    {
    case GuitarHeroGuitarLeftStickX:
        report->leftStickX = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarLeftStickY:
        report->leftStickY = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarWhammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarTilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}

void GuitarHeroGuitarButtonMapping::update_xinput(uint8_t *buf)
{
    XInputGuitarHeroGuitar_Data_t *report = (XInputGuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghAxis)
    {
    case GuitarHeroGuitarLeftStickX:
        // TODO: map to dpad here
        // report->leftStickX = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarLeftStickY:
        // report->leftStickY = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarWhammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarTilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}
void GuitarHeroGuitarButtonMapping::update_ogxbox(uint8_t *buf)
{
    OGXboxGuitarHeroGuitar_Data_t *report = (OGXboxGuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.ghAxis)
    {
    case GuitarHeroGuitarLeftStickX:
        // report->leftStickX = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarLeftStickY:
        // report->leftStickY = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarWhammy:
        report->whammy = m_calibratedValue >> 8;
        break;
    case GuitarHeroGuitarTilt:
        report->tilt = m_calibratedValue >> 8;
        break;
    default:
        break;
    }
}