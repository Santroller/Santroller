#pragma once
#include "state/base.hpp"
class GH5Parser {
   public:
    static inline void parseTapBar(uint8_t val, san_base_t *data) {
        data->guitar.soloGreen = (val == 0x95) || (val == 0xb0) || (val == 0xe5) || (val == 0x19) || (val == 0x2c) || (val == 0x2d) || (val == 0x46) || (val == 0x47) || (val == 0x5f) || (val == 0x60) || (val == 0x61) || (val == 0x62) || (val == 0x78) || (val == 0x79) || (val == 0x7a) || (val == 0x7b);
        data->guitar.soloRed = (val == 0xb0) || (val == 0xcd) || (val == 0xe5) || (val == 0xe6) || (val == 0x2c) || (val == 0x2e) || (val == 0x46) || (val == 0x48) || (val == 0x5f) || (val == 0x60) || (val == 0x63) || (val == 0x64) || (val == 0x78) || (val == 0x79) || (val == 0x7c) || (val == 0x7d);
        data->guitar.soloYellow = (val == 0xe5) || (val == 0xe6) || (val == 0x19) || (val == 0x1a) || (val == 0x2c) || (val == 0x2d) || (val == 0x2e) || (val == 0x2f) || (val == 0x5f) || (val == 0x61) || (val == 0x63) || (val == 0x65) || (val == 0x78) || (val == 0x7a) || (val == 0x7c) || (val == 0x7e);
        data->guitar.soloBlue = (val == 0x2c) || (val == 0x2d) || (val == 0x2e) || (val == 0x2f) || (val == 0x46) || (val == 0x47) || (val == 0x48) || (val == 0x49) || (val == 0x5f) || (val == 0x60) || (val == 0x61) || (val == 0x62) || (val == 0x63) || (val == 0x64) || (val == 0x65) || (val == 0x66);
        data->guitar.soloOrange = (val == 0x5f) || (val == 0x60) || (val == 0x61) || (val == 0x62) || (val == 0x63) || (val == 0x64) || (val == 0x65) || (val == 0x66) || (val == 0x78) || (val == 0x79) || (val == 0x7a) || (val == 0x7b) || (val == 0x7c) || (val == 0x7d) || (val == 0x7e) || (val == 0x7f);
    }
};