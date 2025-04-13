#include "parsers/ps3.hpp"

#include "parsers/dpad.hpp"
#include "parsers/drums.hpp"
#include "parsers/gh5.hpp"
#include "parsers/ghwt.hpp"
#include "parsers/proguitar.hpp"
#include "parsers/prokeys.hpp"
#include "protocols/ps3.hpp"
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
void PS3Parser::parse(uint8_t *report, uint8_t len, san_base_t *data) {
    PS3Dpad_Data_t *dpadData = (PS3Dpad_Data_t *)data;
    switch (subType) {
        case Gamepad: {
            PS3Gamepad_Data_t *report = (PS3Gamepad_Data_t *)data;
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
            if (report->pressureDpadUp) {
                data->gamepad_pressures.pressureDpadUp = report->pressureDpadUp;
            }
            if (report->pressureDpadRight) {
                data->gamepad_pressures.pressureDpadRight = report->pressureDpadRight;
            }
            if (report->pressureDpadDown) {
                data->gamepad_pressures.pressureDpadDown = report->pressureDpadDown;
            }
            if (report->pressureDpadLeft) {
                data->gamepad_pressures.pressureDpadLeft = report->pressureDpadLeft;
            }
            if (report->pressureL1) {
                data->gamepad_pressures.pressureLeftShoulder = report->pressureL1;
            }
            if (report->pressureR1) {
                data->gamepad_pressures.pressureRightShoulder = report->pressureR1;
            }
            if (report->pressureTriangle) {
                data->gamepad_pressures.pressureY = report->pressureTriangle;
            }
            if (report->pressureCircle) {
                data->gamepad_pressures.pressureB = report->pressureCircle;
            }
            if (report->pressureCross) {
                data->gamepad_pressures.pressureA = report->pressureCross;
            }
            if (report->pressureSquare) {
                data->gamepad_pressures.pressureX = report->pressureSquare;
            }
            break;
        }

        case ProKeys: {
            PS3RockBandProKeyboard_Data_t *report = (PS3RockBandProKeyboard_Data_t *)data;
            DpadParser::updateDpad(dpadData->dpad, data);
            ProKeysParser::updateProKeys(report, data, report->pedalAnalog);
            break;
        }
        case RockBandGuitar: {
            PS3RockBandGuitar_Data_t *report = (PS3RockBandGuitar_Data_t *)data;
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
        case GuitarHeroGuitarWt:
        case GuitarHeroGuitar: {
            PS3GuitarHeroGuitar_Data_t *report = (PS3GuitarHeroGuitar_Data_t *)data;
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
            if (report->tilt != PS3_ACCEL_CENTER) {
                data->guitar.tilt = (report->tilt - PS3_ACCEL_CENTER) << 6;
            }
            if (report->whammy) {
                data->guitar.whammy = report->whammy;
            }
            if (subType == GuitarHeroGuitarWt) {
                GHWTParser::parseTapBar(report->slider, data);
            } else {
                GH5Parser::parseTapBar(report->slider, data);
            }
            break;
        }
        case RockBandDrums: {
            PS3RockBandDrums_Data_t *report = (PS3RockBandDrums_Data_t *)data;
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
            DrumParser::updatePadsRB(report, data, data->gamepad.dpadUp, data->gamepad.dpadDown, greenVelocity, redVelocity, yellowVelocity, blueVelocity);
            break;
        }
        case GuitarHeroDrums: {
            PS3GuitarHeroDrums_Data_t *report = (PS3GuitarHeroDrums_Data_t *)data;
            DpadParser::updateDpad(dpadData->dpad, data);
            data->gamepad.a |= report->a;
            data->gamepad.b |= report->b;
            data->gamepad.x |= report->x;
            data->gamepad.y |= report->y;
            data->gamepad.leftShoulder |= report->leftShoulder;
            data->gamepad.back |= report->back;
            data->gamepad.start |= report->start;
            data->gamepad.guide |= report->guide;
            // Forward any midi data we get
            data->midi.midiPacket[0] = report->midiByte0;
            data->midi.midiPacket[1] = report->midiByte1;
            data->midi.midiPacket[2] = report->midiByte2;
            data->midi.midiVelocities[GH_MIDI_NOTE_GREEN] = report->greenVelocity;
            data->midi.midiVelocities[GH_MIDI_NOTE_RED] = report->redVelocity;
            data->midi.midiVelocities[GH_MIDI_NOTE_YELLOW] = report->yellowVelocity;
            data->midi.midiVelocities[GH_MIDI_NOTE_BLUE] = report->blueVelocity;
            data->midi.midiVelocities[GH_MIDI_NOTE_ORANGE] = report->orangeVelocity;
            data->midi.midiVelocities[GH_MIDI_NOTE_KICK] = report->kickVelocity;
            break;
        }
        case LiveGuitar: {
            PS3GHLGuitar_Data_t *report = (PS3GHLGuitar_Data_t *)data;
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
        case DjHeroTurntable: {
            PS3Turntable_Data_t *report = (PS3Turntable_Data_t *)data;
            DpadParser::updateDpad(dpadData->dpad, data);
            data->gamepad.a |= report->a;
            data->gamepad.b |= report->b;
            data->gamepad.x |= report->x;
            data->gamepad.y |= report->y;
            data->gamepad.back |= report->back;
            data->gamepad.start |= report->start;
            data->gamepad.guide |= report->guide;
            data->turntable.leftBlue |= report->leftBlue;
            data->turntable.leftRed |= report->leftRed;
            data->turntable.leftGreen |= report->leftGreen;
            data->turntable.rightBlue |= report->rightBlue;
            data->turntable.rightRed |= report->rightRed;
            data->turntable.rightGreen |= report->rightGreen;
            if (report->effectsKnob != PS3_ACCEL_CENTER) {
                data->turntable.effectsKnob = (report->effectsKnob - PS3_ACCEL_CENTER) << 6;
            }
            if (report->crossfader != PS3_ACCEL_CENTER) {
                data->turntable.crossfader = (report->crossfader - PS3_ACCEL_CENTER) << 6;
            }
            if (report->leftTableVelocity != PS3_STICK_CENTER) {
                data->turntable.leftTableVelocity = (report->leftTableVelocity - PS3_STICK_CENTER) << 8;
            }
            if (report->rightTableVelocity != PS3_STICK_CENTER) {
                data->turntable.rightTableVelocity = (report->rightTableVelocity - PS3_STICK_CENTER) << 8;
            }
            break;
        }

        case ProGuitarSquire:
        case ProGuitarMustang: {
            PS3RockBandProGuitar_Data_t *report = (PS3RockBandProGuitar_Data_t *)data;
            DpadParser::updateDpad(dpadData->dpad, data);
            ProGuitarParser::updateProGuitar(report, data);
            break;
        }
    }
}