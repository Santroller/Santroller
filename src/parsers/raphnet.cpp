#include "parsers/raphnet.hpp"

#include "parsers/dpad.hpp"
#include "parsers/drums.hpp"
#include "parsers/gh5.hpp"
#include "parsers/ghwt.hpp"
#include "parsers/proguitar.hpp"
#include "parsers/prokeys.hpp"
#include "protocols/raphnet.hpp"
#include "string.h"
// TODO: do we do a seperate parser for each device type?
void RaphnetParser::parse(uint8_t *report, uint8_t len, san_base_t *data) {
    switch (mType) {
        case RNT_TYPE_CLASSIC:
        case RNT_TYPE_CLASSIC_PRO: {
            RaphnetGamepad_Data_t *report = (RaphnetGamepad_Data_t *)data;
            data->gamepad.leftStickX = report->leftJoyX - 16000;
            data->gamepad.leftStickY = report->leftJoyY - 16000;
            data->gamepad.rightStickX = report->rightJoyX - 16000;
            data->gamepad.rightStickY = report->rightJoyY - 16000;
            data->gamepad.leftTrigger = report->leftTrigger;
            data->gamepad.rightTrigger = report->rightTrigger;
            data->gamepad.leftShoulder |= report->leftShoulder;
            data->gamepad.rightShoulder |= report->rightShoulder;
            data->gamepad.a |= report->a;
            data->gamepad.b |= report->b;
            data->gamepad.x |= report->x;
            data->gamepad.y |= report->y;
            data->gamepad.start |= report->start;
            data->gamepad.back |= report->select;
            data->gamepad.guide |= report->home;
            data->gamepad.dpadLeft |= report->left;
            data->gamepad.dpadRight |= report->right;
            data->gamepad.dpadUp |= report->up;
            data->gamepad.dpadDown |= report->down;
            break;
        }
        case RNT_TYPE_WII_GUITAR: {
            RaphnetGuitar_Data_t *report = (RaphnetGuitar_Data_t *)data;
            data->gamepad.leftStickX = report->joyX - 16000;
            data->gamepad.leftStickY = report->joyY - 16000;
            data->guitar.whammy = report->whammy >> 8;
            data->gamepad.start = report->plus;
            data->gamepad.back = report->minus;
            data->guitar.green |= report->green;
            data->guitar.red |= report->red;
            data->guitar.yellow |= report->yellow;
            data->guitar.blue |= report->blue;
            data->guitar.orange |= report->orange;
            data->gamepad.a |= report->green;
            data->gamepad.b |= report->red;
            data->gamepad.y |= report->yellow;
            data->gamepad.x |= report->blue;
            data->gamepad.leftShoulder |= report->orange;
            data->gamepad.dpadUp |= report->up;
            data->gamepad.dpadDown |= report->down;
            // TODO: tap bar
            break;
        }
        case RNT_TYPE_WII_DRUM: {
            RaphnetDrum_Data_t *report = (RaphnetDrum_Data_t *)data;
            data->gamepad.leftStickX = report->joyX - 16000;
            data->gamepad.leftStickY = report->joyY - 16000;
            data->gamepad.start = report->plus;
            data->gamepad.back = report->minus;
            // TODO: see if we get all the midi data
            // data->kick1 |= report->kick1;
            data->gamepad.a |= report->green;
            data->gamepad.b |= report->red;
            data->gamepad.y |= report->yellow;
            data->gamepad.x |= report->blue;
            data->gamepad.leftShoulder |= report->orange;
            break;
        }
    }
}