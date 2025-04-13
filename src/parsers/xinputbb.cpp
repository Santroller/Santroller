#include "parsers/xinputbb.hpp"

#include "parsers/drums.hpp"
#include "parsers/gh5.hpp"
#include "parsers/proguitar.hpp"
#include "parsers/prokeys.hpp"
#include "protocols/xinput.hpp"
#include "string.h"

void XInputBigButtonParser::parse(uint8_t *reportData, uint8_t len, san_base_t *data) {
    XInputBigButton_Data_t *report = (XInputBigButton_Data_t *)reportData;
    data->gamepad.a |= report->a;
    data->gamepad.b |= report->b;
    data->gamepad.x |= report->x;
    data->gamepad.y |= report->y;
    data->gamepad.leftShoulder |= report->leftShoulder;
    data->gamepad.rightShoulder |= report->rightShoulder;
    data->gamepad.back |= report->back;
    data->gamepad.start |= report->start;
    data->gamepad.guide |= report->guide;
    data->gamepad.leftThumbClick |= report->leftThumbClick;
    data->gamepad.rightThumbClick |= report->rightThumbClick;
    data->gamepad.dpadLeft = report->dpadLeft;
    data->gamepad.dpadRight = report->dpadRight;
    data->gamepad.dpadUp = report->dpadUp;
    data->gamepad.dpadDown = report->dpadDown;
}