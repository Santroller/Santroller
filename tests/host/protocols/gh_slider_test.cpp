// Xbox 360 Guitar Hero slider decoding in src/devices/usb/host/xinput_tick_helpers.cpp, with the
// values from PlasticBand's Docs/Instruments/5-Fret Guitar/Guitar Hero/General Notes.md
#include <gtest/gtest.h>
#include "pico/time.h"
#include "devices/usb/host/xinput_tick_helpers.h"
#include "protocols/proto_output.hpp"

namespace
{
const GuitarHeroGuitarButtonType taps[] = {GuitarHeroGuitar_TapGreen, GuitarHeroGuitar_TapRed, GuitarHeroGuitar_TapYellow,
                                           GuitarHeroGuitar_TapBlue, GuitarHeroGuitar_TapOrange};

// Tap frets as a GRYBO bitmask, the same layout as gh_slider_helpers.h
uint8_t frets(int16_t slider, bool wt, WtSliderFilter *filter = nullptr)
{
    XInputGuitarHeroGuitar_Data_t data = {};
    data.slider = slider;
    uint8_t out = 0;
    for (int i = 0; i < 5; i++)
    {
        proto_Output o = {};
        o.which_mapping = proto_Output_ghButton_tag;
        o.mapping.ghButton = taps[i];
        if (xinput_tick_digital_impl((const uint8_t *)&data, GuitarHeroGuitar, o, wt, false, filter))
            out |= 1 << i;
    }
    return out;
}

void advance_ms(uint32_t ms) { fake_time::advance_us(ms * 1000ull); }
} // namespace

TEST(GuitarHeroSlider, Gh5UsesTheLowByte)
{
    EXPECT_EQ(frets(0, false), 0);
    EXPECT_EQ(frets((int16_t)0x9495, false), 0b00001);
    EXPECT_EQ(frets((int16_t)0xCCCD, false), 0b00010);
    EXPECT_EQ(frets(0x1A1A, false), 0b00100);
    EXPECT_EQ(frets(0x4949, false), 0b01000);
    EXPECT_EQ(frets(0x7F7F, false), 0b10000);
    EXPECT_EQ(frets((int16_t)0xE4E5, false), 0b00111);
    EXPECT_EQ(frets(0x5F5F, false), 0b11111);
}

TEST(GuitarHeroSlider, WorldTourUsesTheHighByte)
{
    // the top of each range, which rounding the high byte pushed into the next segment
    EXPECT_EQ(frets((int16_t)0x80FF, true), 0b00001);
    EXPECT_EQ(frets((int16_t)0xA2FF, true), 0b00001);
    EXPECT_EQ(frets((int16_t)0xBEFF, true), 0b00011);
    EXPECT_EQ(frets((int16_t)0xD9FF, true), 0b00010);
    EXPECT_EQ(frets((int16_t)0xF2FF, true), 0b00110);
    EXPECT_EQ(frets(0x0CFF, true), 0);
    EXPECT_EQ(frets(0x24FF, true), 0b00100);
    EXPECT_EQ(frets(0x3BFF, true), 0b01100);
    EXPECT_EQ(frets(0x57FF, true), 0b01000);
    EXPECT_EQ(frets(0x72FF, true), 0b11000);
    // orange at the very top of the bar used to wrap around to green
    EXPECT_EQ(frets(0x7FFF, true), 0b10000);
}

TEST(GuitarHeroSlider, WorldTourSegmentsAreCentredOnTheGh5Values)
{
    for (auto [ps3, expected] : {std::pair<uint8_t, uint8_t>{0x15, 0b00001}, {0x30, 0b00011}, {0x4D, 0b00010},
                                 {0x66, 0b00110}, {0x80, 0}, {0x9A, 0b00100}, {0xAF, 0b01100}, {0xC9, 0b01000},
                                 {0xE6, 0b11000}, {0xFF, 0b10000}})
        EXPECT_EQ(decode_ghwt_slider(ps3), expected) << std::hex << (int)ps3;
    // YB on a real guitar read around 0xA4 - 0xB7, the old table stopped YB at 0xAF
    EXPECT_EQ(frets(0x3000, true), 0b01100);
}

// Feeds one report, then polls it until the next one is due, like the host's tick loop
uint8_t report(WtSliderFilter &filter, int16_t raw, uint32_t ms)
{
    uint8_t out = frets(raw, true, &filter);
    for (uint32_t i = 0; i < ms; i++)
    {
        advance_ms(1);
        out = frets(raw, true, &filter);
    }
    return out;
}

TEST(GuitarHeroSlider, WorldTourPressCountsOnceItHolds)
{
    WtSliderFilter filter;
    EXPECT_EQ(frets(0x0000, true, &filter), 0);
    EXPECT_EQ(frets((int16_t)0xCEDB, true, &filter), 0);
    advance_ms(WtSliderFilter::hold_ms - 1);
    EXPECT_EQ(frets((int16_t)0xCEDB, true, &filter), 0);
    advance_ms(1);
    EXPECT_EQ(frets((int16_t)0xCEDB, true, &filter), 0b00010);
    // rest is immediate
    EXPECT_EQ(frets(0x0000, true, &filter), 0);
}

TEST(GuitarHeroSlider, WorldTourPressSkipsTheSweepOutFromRest)
{
    // reports captured from a 360 WT guitar on a receiver, with the gap until the next one
    WtSliderFilter filter;
    report(filter, 0x0000, 100);
    // landing on green read as GR for one report
    EXPECT_EQ(report(filter, (int16_t)0xA56E, 9), 0);
    EXPECT_EQ(report(filter, (int16_t)0x95B7, 100), 0b00001);
    report(filter, 0x0000, 100);
    // and landing on orange as B
    EXPECT_EQ(report(filter, 0x4491, 8), 0);
    EXPECT_EQ(report(filter, 0x7FFF, 100), 0b10000);
}

TEST(GuitarHeroSlider, WorldTourReleaseSkipsTheSweepBackToRest)
{
    WtSliderFilter filter;
    report(filter, (int16_t)0x95B7, 100);
    // letting go of green read as R for one report
    EXPECT_EQ(report(filter, (int16_t)0xD0DB, 7), 0b00001);
    EXPECT_EQ(report(filter, 0x01B6, 100), 0);
    report(filter, 0x7FFF, 100);
    // and letting go of orange as Y
    EXPECT_EQ(report(filter, 0x1092, 7), 0b10000);
    EXPECT_EQ(report(filter, 0x0049, 100), 0);
}

TEST(GuitarHeroSlider, WorldTourSlideFollowsOnceItHolds)
{
    // yellow onto blue, lifting yellow, then rolling back onto it
    WtSliderFilter filter;
    EXPECT_EQ(report(filter, 0x1DFF, 100), 0b00100);
    EXPECT_EQ(report(filter, 0x3324, 100), 0b01100);
    EXPECT_EQ(report(filter, 0x4D23, 23), 0b01000);
    EXPECT_EQ(report(filter, 0x3124, 177), 0b01100);
    EXPECT_EQ(report(filter, 0x4ADA, 100), 0b01000);
}
