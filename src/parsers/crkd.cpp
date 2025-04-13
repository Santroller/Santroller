#include "parsers/crkd.hpp"

#include "parsers/dpad.hpp"
#include "parsers/gh5.hpp"
#include "parsers/proguitar.hpp"
#include "parsers/prokeys.hpp"
#include "protocols/xinput.hpp"
#include "string.h"

void CrkdParser::parse(uint8_t *reportData, uint8_t len, san_base_t *data) {
    XInputCompatGamepad_Data_t *report = (XInputCompatGamepad_Data_t *)data;
    if (report->rid == 2) {
        data->gamepad.guide = reportData[1];
        return;
    }
    if (report->rid == 1) {
        uint8_t dpad = report->dpad - 1;
        DpadParser::updateDpad(dpad, data);
        // Festival pro mode mappings
        data->guitar.green |= data->gamepad.dpadLeft;
        data->guitar.red |= data->gamepad.dpadUp;
        data->guitar.yellow |= report->x;
        data->guitar.blue |= report->y;
        data->guitar.orange |= report->b;
        // Festival expert mappings
        // data->gamepad.green |= data->gamepad.dpadLeft;
        // data->gamepad.red |= data->gamepad.dpadRight;
        // data->gamepad.yellow |= report->x;
        // data->gamepad.blue |= report->y;
        // data->gamepad.orange |= report->b;

        // Festival easy/med/hard mappings
        // data->gamepad.green |= data->gamepad.dpadLeft;
        // data->gamepad.red |= data->gamepad.dpadRight;
        // data->gamepad.yellow |= report->x;
        // data->gamepad.blue |= report->b;
        // data->gamepad.orange |= report->y;
        data->gamepad.a |= report->a;
        data->gamepad.b |= report->b;
        data->gamepad.x |= report->x;
        data->gamepad.y |= report->y;
        data->gamepad.leftShoulder |= report->leftShoulder;
        data->gamepad.rightShoulder |= report->rightShoulder;
        data->gamepad.back |= report->back;
        data->gamepad.start |= report->start;
        data->gamepad.leftThumbClick |= report->leftThumbClick;
        data->gamepad.rightThumbClick |= report->rightThumbClick;
        if (report->leftTrigger) {
            data->gamepad.leftTrigger = report->leftTrigger << 6;
        }
        if (report->rightTrigger) {
            data->gamepad.rightTrigger = report->rightTrigger << 6;
        }
        if (report->leftStickX) {
            data->gamepad.leftStickX = report->leftStickX - INT16_MAX;
        }
        if (report->leftStickY) {
            data->gamepad.leftStickY = -(report->leftStickY - INT16_MAX);
        }
        if (report->rightStickX) {
            data->gamepad.rightStickX = report->rightStickX - INT16_MAX;
        }
        if (report->rightStickY) {
            data->gamepad.rightStickY = -(report->rightStickY - INT16_MAX);
        }
    }
}