#pragma once
#include <stdint.h>
#include <utility>

// Bitmask values for tap frets:
// bit 0 (0b00001): Green
// bit 1 (0b00010): Red
// bit 2 (0b00100): Yellow
// bit 3 (0b01000): Blue
// bit 4 (0b10000): Orange
//
// Both slider generations use the same PS3 8-bit scale, resting at 0x80, with nine positions along
// the bar: each fret, and each pair of neighbouring frets.
//
// World Tour bars are analog and only report a position, so each position owns the range up to the
// midpoints with its neighbours (and with rest).
//
// Guitar Hero 5 bars report the position of the highest fret held (or of the highest pair, when the
// fret below it is held too), minus a bitmask of every fret held below that, nearest fret as bit 0.
// That's at most 7, which never leaves the position's World Tour range, so a GH5 bar reads as its
// top fret / pair to anything that only understands World Tour. This reproduces every value in
// PlasticBand's General Notes.
static constexpr uint8_t gh_slider_rest = 0x80;
// G, GR, R, RY, Y, YB, B, BO, O
static constexpr uint8_t gh_slider_positions[] = {0x15, 0x30, 0x4D, 0x66, 0x9A, 0xAF, 0xC9, 0xE6, 0xFF};
static constexpr uint8_t gh_slider_position_count = sizeof(gh_slider_positions);

// Frets for a position: even positions are a single fret, odd ones the pair either side
constexpr uint8_t gh_slider_position_frets(uint8_t position)
{
    return (position & 1 ? 0b11 : 0b01) << (position / 2);
}

// The fret at the top of a position: position 2n is fret n, 2n - 1 is the pair (n - 1, n)
constexpr uint8_t gh_slider_position_top(uint8_t position)
{
    return (position + 1) / 2;
}

constexpr uint8_t gh5_slider_low_bits(uint8_t top)
{
    return top >= 2 ? top - 1 : 0;
}

// The low bits run the opposite way to the fret bitmask (nearest fret first), so converting between
// them reverses the bottom `bits` bits. There are at most 3.
constexpr uint8_t gh5_slider_reverse(uint8_t value, uint8_t bits)
{
    constexpr uint8_t reversed[] = {0b000, 0b100, 0b010, 0b110, 0b001, 0b101, 0b011, 0b111};
    return reversed[value & 0b111] >> (3 - bits);
}

// World Tour segments, in bar order with rest in the middle: G at 0, rest at 4, O at 9
static constexpr uint8_t ghwt_slider_none = 4;
static constexpr uint8_t ghwt_slider_segment_count = gh_slider_position_count + 1;

constexpr uint8_t ghwt_slider_segment_value(uint8_t seg)
{
    return seg < ghwt_slider_none ? gh_slider_positions[seg] : seg == ghwt_slider_none ? gh_slider_rest
                                                                                       : gh_slider_positions[seg - 1];
}

constexpr uint8_t ghwt_slider_segment_frets(uint8_t seg)
{
    return seg == ghwt_slider_none ? 0 : gh_slider_position_frets(seg < ghwt_slider_none ? seg : seg - 1);
}

// The boundaries are sorted, so the segment is how many of them the value has passed
template <size_t... I>
constexpr uint8_t ghwt_slider_segment(uint8_t slider, std::index_sequence<I...>)
{
    // past the midpoint with the next segment, rounding up
    return ((2 * slider > ghwt_slider_segment_value(I) + ghwt_slider_segment_value(I + 1)) + ...);
}

constexpr uint8_t ghwt_slider_segment(uint8_t slider)
{
    return ghwt_slider_segment(slider, std::make_index_sequence<ghwt_slider_segment_count - 1>());
}

constexpr uint8_t decode_ghwt_slider(uint8_t slider)
{
    return ghwt_slider_segment_frets(ghwt_slider_segment(slider));
}

// The value sweeps through the segments in between whenever a finger lands or lifts (pressing
// orange briefly reads as B, letting go of green as R), each for a single report (~8ms on a 360
// receiver). A new segment has to hold for hold_ms before it counts, so the next report gets a
// chance to contradict it. Guitars only report on change, so this is timed rather than counting
// reports. Rest is taken immediately, so releases aren't delayed.
struct WtSliderFilter
{
    static constexpr uint32_t hold_ms = 12;
    static constexpr uint8_t none = ghwt_slider_none;
    uint8_t stable = none;
    uint8_t pending = none;
    uint32_t pending_since = 0;

    void reset()
    {
        stable = none;
        pending = none;
    }

    uint8_t update(uint8_t slider, uint32_t now)
    {
        uint8_t seg = ghwt_slider_segment(slider);
        if (seg == none)
        {
            stable = none;
            pending = none;
        }
        else if (seg != pending)
        {
            pending = seg;
            pending_since = now;
        }
        else if (seg != stable && now - pending_since >= hold_ms)
        {
            stable = seg;
        }
        return ghwt_slider_segment_frets(stable);
    }
};

// The lowest position at or above the value, since the low bits only ever subtract. O is 0xFF, so
// there always is one.
template <size_t... I>
constexpr uint8_t gh5_slider_position(uint8_t slider, std::index_sequence<I...>)
{
    return ((slider > gh_slider_positions[I]) + ...);
}

// Values that aren't a valid GH5 encoding (including rest) decode as no frets
constexpr uint8_t decode_gh5_slider(uint8_t slider)
{
    uint8_t position = gh5_slider_position(slider, std::make_index_sequence<gh_slider_position_count>());
    uint8_t bits = gh5_slider_low_bits(gh_slider_position_top(position));
    uint8_t low = gh_slider_positions[position] - slider;
    if (low >> bits)
        return 0;
    return gh_slider_position_frets(position) | gh5_slider_reverse(low, bits);
}

constexpr uint8_t encode_gh5_slider(uint8_t frets)
{
    frets &= 0b11111;
    if (!frets)
        return gh_slider_rest;
    uint8_t top = 31 - __builtin_clz(frets);
    bool pair = top && (frets & (1 << (top - 1)));
    uint8_t position = pair ? 2 * top - 1 : 2 * top;
    uint8_t bits = gh5_slider_low_bits(top);
    return gh_slider_positions[position] - gh5_slider_reverse(frets & ((1 << bits) - 1), bits);
}
