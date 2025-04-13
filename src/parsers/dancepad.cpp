#include "parsers/dancepad.hpp"

#include "parsers/drums.hpp"
#include "parsers/gh5.hpp"
#include "parsers/proguitar.hpp"
#include "parsers/prokeys.hpp"
#include "protocols/dance_pad.hpp"
#include "string.h"

void DancepadParser::parse(uint8_t *report, uint8_t len, san_base_t *data) {
    switch (mType) {
        case LTEK_ID: {
            LTEK_Report_With_Id_Data_t *report = (LTEK_Report_With_Id_Data_t *)data;
            data->gamepad.dpadLeft |= report->dpadLeft;
            data->gamepad.dpadRight |= report->dpadRight;
            data->gamepad.dpadUp |= report->dpadUp;
            data->gamepad.dpadDown |= report->dpadDown;
            data->gamepad.start |= report->start;
            data->gamepad.back |= report->back;
            break;
        }
        case LTEK: {
            LTEK_Report_Data_t *report = (LTEK_Report_Data_t *)data;
            data->gamepad.dpadLeft |= report->dpadLeft;
            data->gamepad.dpadRight |= report->dpadRight;
            data->gamepad.dpadUp |= report->dpadUp;
            data->gamepad.dpadDown |= report->dpadDown;
            data->gamepad.start |= report->start;
            data->gamepad.back |= report->back;
            break;
        }
        case STEPMANIAX: {
            StepManiaX_Report_Data_t *report = (StepManiaX_Report_Data_t *)data;
            data->gamepad.dpadLeft |= report->dpadLeft;
            data->gamepad.dpadRight |= report->dpadRight;
            data->gamepad.dpadUp |= report->dpadUp;
            data->gamepad.dpadDown |= report->dpadDown;
            break;
        }
    }
}