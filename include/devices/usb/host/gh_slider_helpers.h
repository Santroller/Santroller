#pragma once
#include <stdint.h>

// Bitmask values for decoded tap frets:
// bit 0 (0b00001): Green
// bit 1 (0b00010): Red
// bit 2 (0b00100): Yellow
// bit 3 (0b01000): Blue
// bit 4 (0b10000): Orange

// World Tour bars are analog, so each segment is a range on the PS3 8-bit scale (resting at
// 0x80). The guitars sit around the GH5 "top frets" base values from PlasticBand's General Notes
// (G 0x15, GR 0x30, R 0x4D, RY 0x66, Y 0x9A, YB 0xAF, B 0xC9, BO 0xE6, O 0xFF), so each
// boundary is the midpoint between neighbouring ones. PlasticBand's ordered-by-value table ends
// YB at 0xAF, its own centre, which turned real YB readings into B.
// Segments are in bar order, G at 0 and O at 9, with rest in the middle.
static constexpr uint8_t ghwt_slider_none = 4;
static constexpr uint8_t ghwt_slider_frets[] = {
    0b00001, // G
    0b00011, // GR
    0b00010, // R
    0b00110, // RY
    0,       // None
    0b00100, // Y
    0b01100, // YB
    0b01000, // B
    0b11000, // BO
    0b10000, // O
};

inline uint8_t ghwt_slider_segment(uint8_t slider)
{
    // first value of each segment after G
    static constexpr uint8_t starts[] = {0x23, 0x3F, 0x5A, 0x73, 0x8D, 0xA5, 0xBC, 0xD8, 0xF3};
    uint8_t seg = 0;
    while (seg < sizeof(starts) && slider >= starts[seg])
        seg++;
    return seg;
}

inline uint8_t decode_ghwt_slider(uint8_t slider)
{
    return ghwt_slider_frets[ghwt_slider_segment(slider)];
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
        return ghwt_slider_frets[stable];
    }
};

inline uint8_t decode_gh5_slider(uint8_t slider)
{
    // PlasticBand Guitar Hero 5 discrete values:
    switch (slider)
    {
    // Single frets
    case 0x15: return 0b00001; // G
    case 0x4D: return 0b00010; // R
    case 0x9A: return 0b00100; // Y
    case 0xC9: return 0b01000; // B
    case 0xFF: return 0b10000; // O

    // 2-fret combinations
    case 0x30: return 0b00011; // GR
    case 0x99: return 0b00101; // GY
    case 0xC7: return 0b01001; // GB
    case 0xFB: return 0b10001; // GO
    case 0x66: return 0b00110; // RY
    case 0xC8: return 0b01010; // RB
    case 0xFD: return 0b10010; // RO
    case 0xAF: return 0b01100; // YB
    case 0xFE: return 0b10100; // YO
    case 0xE6: return 0b11000; // BO

    // 3-fret combinations
    case 0x65: return 0b00111; // GRY
    case 0xC6: return 0b01011; // GRB
    case 0xF9: return 0b10011; // GRO
    case 0xAD: return 0b01101; // GYB
    case 0xFA: return 0b10101; // GYO
    case 0xE2: return 0b11001; // GBO
    case 0xAE: return 0b01110; // RYB
    case 0xFC: return 0b10110; // RYO
    case 0xE4: return 0b11010; // RBO
    case 0xE5: return 0b11100; // YBO

    // 4-fret combinations
    case 0xAC: return 0b01111; // GRYB
    case 0xF8: return 0b10111; // GRYO
    case 0xE0: return 0b11011; // GRBO
    case 0xE1: return 0b11101; // GYBO
    case 0xE3: return 0b11110; // RYBO

    // 5-fret combination
    case 0xDF: return 0b11111; // GRYBO

    default:   return 0;       // None (0x80)
    }
}
