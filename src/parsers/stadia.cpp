#include "parsers/stadia.hpp"

#include "parsers/dpad.hpp"
#include "parsers/drums.hpp"
#include "parsers/gh5.hpp"
#include "parsers/ghwt.hpp"
#include "parsers/proguitar.hpp"
#include "parsers/prokeys.hpp"
#include "protocols/controller_reports.hpp"
#include "string.h"
void StadiaParser::parse(uint8_t *reportData, uint8_t len, san_base_t *data) {
    Stadia_Data_t *report = (Stadia_Data_t *)reportData;
    DpadParser::updateDpad(report->dpad, data);
    data->gamepad.a |= report->a;
    data->gamepad.b |= report->b;
    data->gamepad.x |= report->x;
    data->gamepad.y |= report->y;
    data->gamepad.capture |= report->capture;
    data->gamepad.leftShoulder |= report->leftShoulder;
    data->gamepad.rightShoulder |= report->rightShoulder;
    data->gamepad.back |= report->back;
    data->gamepad.start |= report->start;
    data->gamepad.guide |= report->guide;
    data->gamepad.leftThumbClick |= report->leftThumbClick;
    data->gamepad.rightThumbClick |= report->rightThumbClick;
    if (report->leftTrigger) {
        data->gamepad.leftTrigger = report->leftTrigger << 8;
    }
    if (report->rightTrigger) {
        data->gamepad.rightTrigger = report->rightTrigger << 8;
    }
    if (report->leftStickX != PS3_STICK_CENTER) {
        data->gamepad.leftStickX = (report->leftStickX - PS3_STICK_CENTER) << 8;
    }
    if (report->leftStickY != PS3_STICK_CENTER) {
        data->gamepad.leftStickY = (((UINT8_MAX - report->leftStickY) - PS3_STICK_CENTER)) << 8;
    }
    if (report->rightStickX != PS3_STICK_CENTER) {
        data->gamepad.rightStickX = (report->rightStickX - PS3_STICK_CENTER) << 8;
    }
    if (report->rightStickY != PS3_STICK_CENTER) {
        data->gamepad.rightStickY = (((UINT8_MAX - report->rightStickY) - PS3_STICK_CENTER)) << 8;
    }
}