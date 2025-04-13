#include "devices/gh5_neck.hpp"

#include "interfaces/core.hpp"

void Gh5NeckDevice::tick(san_base_t* data) {
    uint8_t gh5Data[2];
    mFound = mInterface->readFromPointer(GH5NECK_ADDR, GH5NECK_BUTTONS_PTR, sizeof(gh5Data), gh5Data);
    if (mFound) {
        data->guitar.green = gh5Data[0] & 1 << 4;
        data->guitar.red = gh5Data[0] & 1 << 5;
        data->guitar.yellow = gh5Data[0] & 1 << 6;
        data->guitar.blue = gh5Data[0] & 1 << 7;
        data->guitar.orange = gh5Data[0] & 1 << 0;
        data->guitar.soloGreen = (gh5Data[1] == 0x95) || (gh5Data[1] == 0xb0) || (gh5Data[1] == 0xe5) || (gh5Data[1] == 0x19) || (gh5Data[1] == 0x2c) || (gh5Data[1] == 0x2d) || (gh5Data[1] == 0x46) || (gh5Data[1] == 0x47) || (gh5Data[1] == 0x5f) || (gh5Data[1] == 0x60) || (gh5Data[1] == 0x61) || (gh5Data[1] == 0x62) || (gh5Data[1] == 0x78) || (gh5Data[1] == 0x79) || (gh5Data[1] == 0x7a) || (gh5Data[1] == 0x7b);
        data->guitar.soloRed = (gh5Data[1] == 0xb0) || (gh5Data[1] == 0xcd) || (gh5Data[1] == 0xe5) || (gh5Data[1] == 0xe6) || (gh5Data[1] == 0x2c) || (gh5Data[1] == 0x2e) || (gh5Data[1] == 0x46) || (gh5Data[1] == 0x48) || (gh5Data[1] == 0x5f) || (gh5Data[1] == 0x60) || (gh5Data[1] == 0x63) || (gh5Data[1] == 0x64) || (gh5Data[1] == 0x78) || (gh5Data[1] == 0x79) || (gh5Data[1] == 0x7c) || (gh5Data[1] == 0x7d);
        data->guitar.soloYellow = (gh5Data[1] == 0xe5) || (gh5Data[1] == 0xe6) || (gh5Data[1] == 0x19) || (gh5Data[1] == 0x1a) || (gh5Data[1] == 0x2c) || (gh5Data[1] == 0x2d) || (gh5Data[1] == 0x2e) || (gh5Data[1] == 0x2f) || (gh5Data[1] == 0x5f) || (gh5Data[1] == 0x61) || (gh5Data[1] == 0x63) || (gh5Data[1] == 0x65) || (gh5Data[1] == 0x78) || (gh5Data[1] == 0x7a) || (gh5Data[1] == 0x7c) || (gh5Data[1] == 0x7e);
        data->guitar.soloBlue = (gh5Data[1] == 0x2c) || (gh5Data[1] == 0x2d) || (gh5Data[1] == 0x2e) || (gh5Data[1] == 0x2f) || (gh5Data[1] == 0x46) || (gh5Data[1] == 0x47) || (gh5Data[1] == 0x48) || (gh5Data[1] == 0x49) || (gh5Data[1] == 0x5f) || (gh5Data[1] == 0x60) || (gh5Data[1] == 0x61) || (gh5Data[1] == 0x62) || (gh5Data[1] == 0x63) || (gh5Data[1] == 0x64) || (gh5Data[1] == 0x65) || (gh5Data[1] == 0x66);
        data->guitar.soloOrange = (gh5Data[1] == 0x5f) || (gh5Data[1] == 0x60) || (gh5Data[1] == 0x61) || (gh5Data[1] == 0x62) || (gh5Data[1] == 0x63) || (gh5Data[1] == 0x64) || (gh5Data[1] == 0x65) || (gh5Data[1] == 0x66) || (gh5Data[1] == 0x78) || (gh5Data[1] == 0x79) || (gh5Data[1] == 0x7a) || (gh5Data[1] == 0x7b) || (gh5Data[1] == 0x7c) || (gh5Data[1] == 0x7d) || (gh5Data[1] == 0x7e) || (gh5Data[1] == 0x7f);
    }
}