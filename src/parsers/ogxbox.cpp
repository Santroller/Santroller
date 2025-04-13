#include "parsers/ogxbox.hpp"

#include "parsers/dpad.hpp"
#include "parsers/drums.hpp"
#include "parsers/gh5.hpp"
#include "parsers/ghwt.hpp"
#include "parsers/proguitar.hpp"
#include "parsers/prokeys.hpp"
#include "protocols/og_xbox.hpp"
#include "string.h"
void OgXboxParser::parse(uint8_t *reportData, uint8_t len, san_base_t *data) {
    OGXboxGamepad_Data_t *report = (OGXboxGamepad_Data_t *)data;
    data->gamepad.a = report->a > 0x20;
    data->gamepad.b = report->b > 0x20;
    data->gamepad.x = report->x > 0x20;
    data->gamepad.y = report->y > 0x20;
    data->gamepad.leftShoulder = report->leftShoulder > 0x20;
    data->gamepad.rightShoulder = report->rightShoulder > 0x20;
    data->gamepad.back = report->back;
    data->gamepad.start = report->start;
    data->gamepad.leftThumbClick = report->leftThumbClick;
    data->gamepad.rightThumbClick = report->rightThumbClick;
    data->gamepad.dpadLeft = report->dpadLeft;
    data->gamepad.dpadRight = report->dpadRight;
    data->gamepad.dpadUp = report->dpadUp;
    data->gamepad.dpadDown = report->dpadDown;
    data->gamepad.leftTrigger = report->leftTrigger << 8;
    data->gamepad.rightTrigger = report->rightTrigger << 8;
    data->gamepad.leftStickX = report->leftStickX;
    data->gamepad.leftStickY = report->leftStickY;
    data->gamepad.rightStickX = report->rightStickX;
    data->gamepad.rightStickY = report->rightStickY;
    data->gamepad_pressures.pressureLeftShoulder = report->leftShoulder;
    data->gamepad_pressures.pressureRightShoulder = report->rightShoulder;
    data->gamepad_pressures.pressureY = report->y;
    data->gamepad_pressures.pressureB = report->b;
    data->gamepad_pressures.pressureA = report->a;
    data->gamepad_pressures.pressureX = report->x;
}