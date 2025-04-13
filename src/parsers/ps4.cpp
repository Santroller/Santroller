#include "parsers/ps4.hpp"

#include "parsers/dpad.hpp"
#include "parsers/drums.hpp"
#include "parsers/gh5.hpp"
#include "parsers/ghwt.hpp"
#include "parsers/proguitar.hpp"
#include "parsers/prokeys.hpp"
#include "protocols/ps4.hpp"
#include "string.h"

static inline uint8_t xinput_rb_velocity_positive(int16_t velocity) {
    return (uint8_t)(255 - (uint8_t)(velocity >> 7));
}
static inline uint8_t xinput_rb_velocity_negative(int16_t velocity) {
    if (velocity == 0)
        return 255;

    return (uint8_t)(255 - (uint8_t)((~velocity) >> 7));
}

// TODO: do we do a seperate parser for each device type?
void PS4Parser::parse(uint8_t *report, uint8_t len, san_base_t *data) {
    PS4Dpad_Data_t *dpadData = (PS4Dpad_Data_t *)data;
    switch (subType) {
        case Gamepad: {
            PS4Gamepad_Data_t *report = (PS4Gamepad_Data_t *)data;
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
            data->gamepad.dpadLeft |= report->dpadLeft;
            data->gamepad.dpadRight |= report->dpadRight;
            data->gamepad.dpadUp |= report->dpadUp;
            data->gamepad.dpadDown |= report->dpadDown;
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
            PS4RockBandGuitar_Data_t *report = (PS4RockBandGuitar_Data_t *)data;
            DpadParser::updateDpad(dpadData->dpad, data);
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
            if (report->pickup) {
                data->guitar.pickup = report->pickup;
            }
            break;
        }
        case RockBandDrums: {
            PS4RockBandDrums_Data_t *report = (PS4RockBandDrums_Data_t *)data;
            uint8_t redVelocity = ~report->redVelocity;
            uint8_t greenVelocity = ~report->greenVelocity;
            uint8_t yellowVelocity = ~report->yellowVelocity;
            uint8_t blueVelocity = ~report->blueVelocity;
            bool kick1 = report->leftShoulder;
            bool kick2 = report->rightShoulder;
            data->gamepad.a |= report->a;
            data->gamepad.b |= report->b;
            data->gamepad.x |= report->x;
            data->gamepad.y |= report->y;
            data->gamepad.back |= report->back;
            data->gamepad.start |= report->start;
            data->gamepad.guide |= report->guide;
            DpadParser::updateDpad(dpadData->dpad, data);
            data->midi.midiVelocities[RB_MIDI_NOTE_GREEN] = report->greenVelocity >> 1;
            data->midi.midiVelocities[RB_MIDI_NOTE_RED] = report->redVelocity >> 1;
            data->midi.midiVelocities[RB_MIDI_NOTE_YELLOW] = report->yellowVelocity >> 1;
            data->midi.midiVelocities[RB_MIDI_NOTE_BLUE] = report->blueVelocity >> 1;
            data->midi.midiVelocities[RB_MIDI_NOTE_GREEN] = report->greenCymbalVelocity >> 1;
            data->midi.midiVelocities[RB_MIDI_NOTE_YELLOW] = report->yellowCymbalVelocity >> 1;
            data->midi.midiVelocities[RB_MIDI_NOTE_BLUE] = report->blueCymbalVelocity >> 1;
            data->midi.midiVelocities[RB_MIDI_NOTE_KICK] = kick1 ? 0xFF : 0x00;
            data->midi.midiVelocities[RB_MIDI_NOTE_KICK2] = kick2 ? 0xFF : 0x00;
            break;
        }
        case LiveGuitar: {
            PS4GHLGuitar_Data_t *report = (PS4GHLGuitar_Data_t *)data;
            DpadParser::updateDpad(dpadData->dpad, data);
            data->gamepad.a |= report->a;
            data->gamepad.b |= report->b;
            data->gamepad.x |= report->x;
            data->gamepad.y |= report->y;
            data->gamepad.leftShoulder |= report->leftShoulder;
            data->gamepad.rightShoulder |= report->rightShoulder;
            data->live_guitar.strumDown = report->strumBar == 0xFF;
            data->live_guitar.strumUp = report->strumBar == 0x00;
            data->live_guitar.ghtv = report->leftThumbClick;
            data->live_guitar.heroPower = report->back;
            data->live_guitar.black1 = report->a;
            data->live_guitar.black2 = report->b;
            data->live_guitar.black3 = report->y;
            data->live_guitar.white1 = report->x;
            data->live_guitar.white2 = report->leftShoulder;
            data->live_guitar.white3 = report->rightShoulder;
            data->gamepad.leftThumbClick |= report->leftThumbClick;
            data->gamepad.back |= report->back;
            data->gamepad.start |= report->start;
            data->gamepad.guide |= report->guide;
            if (report->tilt != PS3_ACCEL_CENTER) {
                data->live_guitar.tilt = (report->tilt - PS3_ACCEL_CENTER) << 6;
            }
            if (report->whammy) {
                data->live_guitar.whammy = report->whammy << 8;
            }
            break;
        }
    }
}