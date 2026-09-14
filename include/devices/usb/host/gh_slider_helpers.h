#pragma once
#include <stdint.h>

// Bitmask values for decoded tap frets:
// bit 0 (0b00001): Green
// bit 1 (0b00010): Red
// bit 2 (0b00100): Yellow
// bit 3 (0b01000): Blue
// bit 4 (0b10000): Orange

inline uint8_t decode_ghwt_slider(uint8_t slider)
{
    // PlasticBand World Tour ranges (PS3 8-bit scale resting at 0x80):
    // G:     0x00 - 0x2F
    // GR:    0x30 - 0x3F
    // R:     0x40 - 0x5F
    // RY:    0x60 - 0x6F
    // None:  0x70 - 0x8F
    // Y:     0x90 - 0x9F
    // YB:    0xA0 - 0xBF
    // B:     0xC0 - 0xCF
    // BO:    0xD0 - 0xEF
    // O:     0xF0 - 0xFF
    if (slider <= 0x2F) return 0b00001;      // G
    if (slider <= 0x3F) return 0b00011;      // GR
    if (slider <= 0x5F) return 0b00010;      // R
    if (slider <= 0x6F) return 0b00110;      // RY
    if (slider <= 0x8F) return 0;            // None
    if (slider <= 0x9F) return 0b00100;      // Y
    if (slider <= 0xAF) return 0b01100;      // YB
    if (slider <= 0xCF) return 0b01000;      // B
    if (slider <= 0xEF) return 0b11000;      // BO
    return 0b10000;                          // O (0xF0 - 0xFF)
}

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
