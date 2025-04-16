#include "parsers/ps5.hpp"

#include "parsers/dpad.hpp"
#include "parsers/drums.hpp"
#include "parsers/gh5.hpp"
#include "parsers/ghwt.hpp"
#include "parsers/proguitar.hpp"
#include "parsers/prokeys.hpp"
#include "protocols/ps5.hpp"
#include "string.h"
#include "stdint.h"
// TODO: do we do a seperate parser for each device type?
void PS5Parser::parse(uint8_t *report, uint8_t len, san_base_t *data) {
    switch (subType) {
        case Gamepad: {
            PS5Gamepad_Data_t *report = (PS5Gamepad_Data_t *)data;
            DpadParser::updateDpad(report->dpad, data);
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
            break;
        }

        case RockBandGuitar: {
            PS5RockBandGuitar_Data_t *report = (PS5RockBandGuitar_Data_t *)data;
            DpadParser::updateDpad(report->dpad, data);
            data->gamepad.a |= report->a;
            data->gamepad.b |= report->b;
            data->gamepad.x |= report->x;
            data->gamepad.y |= report->y;
            data->gamepad.leftShoulder |= report->leftShoulder;
            data->guitar.green |= report->a;
            data->guitar.red |= report->b;
            data->guitar.yellow |= report->y;
            data->guitar.blue |= report->x;
            data->guitar.orange |= report->leftShoulder;
            data->gamepad.back |= report->back;
            data->gamepad.start |= report->start;
            data->gamepad.guide |= report->guide;
            if (report->tilt) {
                data->guitar.tilt = INT16_MAX;
            }
            if (report->solo) {
                data->guitar.soloGreen |= report->a;
                data->guitar.soloRed |= report->b;
                data->guitar.soloYellow |= report->y;
                data->guitar.soloBlue |= report->x;
                data->guitar.soloOrange |= report->leftShoulder;
            }
            if (report->whammy) {
                data->guitar.whammy = report->whammy;
            }
            break;
        }
    }
}