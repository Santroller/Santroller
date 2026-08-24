#include "mappings/mapping.hpp"
#include "instance.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include <pb_encode.h>
#include <utils.h>
#include <stdint.h>

GuitarHeroArcadeAxisMapping::GuitarHeroArcadeAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : AxisMapping(mapping, std::move(input), id, profile, true)
{
}

void GuitarHeroArcadeAxisMapping::update_hid(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    ArcadeGuitarHeroGuitar_Data_t *report = (ArcadeGuitarHeroGuitar_Data_t *)buf;
    switch (m_mapping.mapping.mapping.ghaAxis)
    {
    case GuitarHeroArcade_Tilt:
        report->tilt = m_calibratedValue - 32768;
        break;
    }
}
void GuitarHeroArcadeAxisMapping::update_wii(uint8_t format, uint8_t *buf)
{
}
void GuitarHeroArcadeAxisMapping::update_switch(uint8_t *buf)
{
}

void GuitarHeroArcadeAxisMapping::update_ps2(uint8_t *buf)
{
}

void GuitarHeroArcadeAxisMapping::update_ps3(uint8_t *buf)
{
}

void GuitarHeroArcadeAxisMapping::update_ps4(uint8_t *buf)
{
}

void GuitarHeroArcadeAxisMapping::update_ps5(uint8_t *buf)
{
}

void GuitarHeroArcadeAxisMapping::update_xinput(uint8_t *buf)
{
}
void GuitarHeroArcadeAxisMapping::update_ogxbox(uint8_t *buf)
{
}
void GuitarHeroArcadeAxisMapping::update_xboxone(uint8_t *buf)
{
}