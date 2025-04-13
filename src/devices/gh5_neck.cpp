#include "devices/gh5_neck.hpp"
#include "parsers/gh5.hpp"

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
        GH5Parser::parseTapBar(gh5Data[1], data);
     
    }
};