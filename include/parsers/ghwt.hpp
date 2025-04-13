#pragma once
#include "state/base.hpp"
class GHWTParser {
   public:
    static inline void parseTapBar(uint8_t val, san_base_t *data) {
        if (val < 0x2F) {
            data->guitar.soloGreen = true;
            data->guitar.soloRed = false;
            data->guitar.soloYellow = false;
            data->guitar.soloBlue = false;
            data->guitar.soloOrange = false;
        } else if (val <= 0x3F) {
            data->guitar.soloGreen = true;
            data->guitar.soloRed = true;
            data->guitar.soloYellow = false;
            data->guitar.soloBlue = false;
            data->guitar.soloOrange = false;
        } else if (val <= 0x5F) {
            data->guitar.soloGreen = false;
            data->guitar.soloRed = true;
            data->guitar.soloYellow = false;
            data->guitar.soloBlue = false;
            data->guitar.soloOrange = false;
        } else if (val <= 0x6F) {
            data->guitar.soloGreen = false;
            data->guitar.soloRed = true;
            data->guitar.soloYellow = true;
            data->guitar.soloBlue = false;
            data->guitar.soloOrange = false;
        } else if (val <= 0x8F) {
            val = 0x80;
        } else if (val <= 0x9F) {
            data->guitar.soloGreen = false;
            data->guitar.soloRed = false;
            data->guitar.soloYellow = true;
            data->guitar.soloBlue = false;
            data->guitar.soloOrange = false;
        } else if (val <= 0xAF) {
            data->guitar.soloGreen = false;
            data->guitar.soloRed = false;
            data->guitar.soloYellow = true;
            data->guitar.soloBlue = true;
            data->guitar.soloOrange = false;
        } else if (val <= 0xCF) {
            data->guitar.soloGreen = false;
            data->guitar.soloRed = false;
            data->guitar.soloYellow = false;
            data->guitar.soloBlue = true;
            data->guitar.soloOrange = false;
        } else if (val <= 0xEF) {
            data->guitar.soloGreen = false;
            data->guitar.soloRed = false;
            data->guitar.soloYellow = false;
            data->guitar.soloBlue = true;
            data->guitar.soloOrange = true;
        } else {
            data->guitar.soloGreen = false;
            data->guitar.soloRed = false;
            data->guitar.soloYellow = false;
            data->guitar.soloBlue = false;
            data->guitar.soloOrange = true;
        }
    }
};