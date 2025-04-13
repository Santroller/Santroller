#include "parsers/xinput.hpp"

#include "parsers/drums.hpp"
#include "parsers/gh5.hpp"
#include "parsers/proguitar.hpp"
#include "parsers/prokeys.hpp"
#include "protocols/xinput.hpp"
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
void XInputParser::parse(uint8_t *report, uint8_t len, san_base_t *data) {
    switch (subType) {
        case XINPUT_GUITAR_BASS:
        case XINPUT_GUITAR: {
            XInputRockBandGuitar_Data_t *report = (XInputRockBandGuitar_Data_t *)data;
            data->gamepad.a = report->a;
            data->gamepad.b = report->b;
            data->gamepad.x = report->x;
            data->gamepad.y = report->y;
            data->gamepad.leftShoulder = report->leftShoulder;
            data->guitar.green = report->a;
            data->guitar.red = report->b;
            data->guitar.yellow = report->y;
            data->guitar.blue = report->x;
            data->guitar.orange = report->leftShoulder;
            data->gamepad.back = report->back;
            data->gamepad.start = report->start;
            data->gamepad.guide = report->guide;
            data->gamepad.dpadLeft = report->dpadLeft;
            data->gamepad.dpadRight = report->dpadRight;
            data->gamepad.dpadUp = report->dpadUp;
            data->gamepad.dpadDown = report->dpadDown;
            if (report->tilt) {
                data->guitar.tilt = INT16_MAX;
            }
            if (report->solo) {
                data->guitar.soloGreen = report->a;
                data->guitar.soloRed = report->b;
                data->guitar.soloYellow = report->y;
                data->guitar.soloBlue = report->x;
                data->guitar.soloOrange = report->leftShoulder;
            }
            if (report->whammy) {
                data->guitar.whammy = (report->whammy >> 8) - PS3_STICK_CENTER;
            }
            if (report->pickup) {
                data->guitar.pickup = report->pickup;
            }
            break;
        }
        case XINPUT_GUITAR_WT:
        case XINPUT_GUITAR_ALTERNATE: {
            XInputGuitarHeroGuitar_Data_t *report = (XInputGuitarHeroGuitar_Data_t *)data;
            data->gamepad.a = report->a;
            data->gamepad.b = report->b;
            data->gamepad.x = report->x;
            data->gamepad.y = report->y;
            data->gamepad.leftShoulder = report->leftShoulder;
            data->guitar.green = report->a;
            data->guitar.red = report->b;
            data->guitar.yellow = report->y;
            data->guitar.blue = report->x;
            data->guitar.orange = report->leftShoulder;
            data->gamepad.back = report->back;
            data->gamepad.start = report->start;
            data->gamepad.guide = report->guide;
            data->gamepad.dpadLeft = report->dpadLeft;
            data->gamepad.dpadRight = report->dpadRight;
            data->gamepad.dpadUp = report->dpadUp;
            data->gamepad.dpadDown = report->dpadDown;
            if (report->tilt) {
                data->guitar.tilt = report->tilt;
            }
            if (report->whammy) {
                data->guitar.whammy = (report->whammy >> 8) - PS3_STICK_CENTER;
            }

            uint8_t slider = (report->slider >> 8);

            if (subType == XINPUT_GUITAR_WT) {
                if (slider < 0x2F) {
                    data->guitar.soloGreen = true;
                    data->guitar.soloRed = false;
                    data->guitar.soloYellow = false;
                    data->guitar.soloBlue = false;
                    data->guitar.soloOrange = false;
                } else if (slider <= 0x3F) {
                    data->guitar.soloGreen = true;
                    data->guitar.soloRed = true;
                    data->guitar.soloYellow = false;
                    data->guitar.soloBlue = false;
                    data->guitar.soloOrange = false;
                } else if (slider <= 0x5F) {
                    data->guitar.soloGreen = false;
                    data->guitar.soloRed = true;
                    data->guitar.soloYellow = false;
                    data->guitar.soloBlue = false;
                    data->guitar.soloOrange = false;
                } else if (slider <= 0x6F) {
                    data->guitar.soloGreen = false;
                    data->guitar.soloRed = true;
                    data->guitar.soloYellow = true;
                    data->guitar.soloBlue = false;
                    data->guitar.soloOrange = false;
                } else if (slider <= 0x8F) {
                    slider = 0x80;
                } else if (slider <= 0x9F) {
                    data->guitar.soloGreen = false;
                    data->guitar.soloRed = false;
                    data->guitar.soloYellow = true;
                    data->guitar.soloBlue = false;
                    data->guitar.soloOrange = false;
                } else if (slider <= 0xAF) {
                    data->guitar.soloGreen = false;
                    data->guitar.soloRed = false;
                    data->guitar.soloYellow = true;
                    data->guitar.soloBlue = true;
                    data->guitar.soloOrange = false;
                } else if (slider <= 0xCF) {
                    data->guitar.soloGreen = false;
                    data->guitar.soloRed = false;
                    data->guitar.soloYellow = false;
                    data->guitar.soloBlue = true;
                    data->guitar.soloOrange = false;
                } else if (slider <= 0xEF) {
                    data->guitar.soloGreen = false;
                    data->guitar.soloRed = false;
                    data->guitar.soloYellow = false;
                    data->guitar.soloBlue = true;
                    data->guitar.soloOrange = true;
                } else {
                    data->guitar.soloGreen = false;
                    data->guitar.soloRed = false;
                    data->guitar.soloYellow = false;
                    data->guitar.soloBlue = false;
                    data->guitar.soloOrange = true;
                }
            } else {
                GH5Parser::parseTapBar(slider, data);
            }
            break;
        }
        case XINPUT_RB_DRUMS: {
            XInputRockBandDrums_Data_t *report = (XInputRockBandDrums_Data_t *)data;
            bool up = report->dpadUp;
            bool down = report->dpadDown;
            bool left = report->dpadLeft;
            bool right = report->dpadRight;
            bool kick1 = report->leftShoulder;
            bool kick2 = report->leftThumbClick;

            uint8_t redVelocity = xinput_rb_velocity_positive(report->redVelocity);
            uint8_t greenVelocity = xinput_rb_velocity_negative(report->greenVelocity);
            uint8_t yellowVelocity = xinput_rb_velocity_negative(report->yellowVelocity);
            uint8_t blueVelocity = xinput_rb_velocity_positive(report->blueVelocity);

            DrumParser::updatePadsRB(report, data, report->dpadUp, report->dpadDown, greenVelocity, redVelocity, yellowVelocity, blueVelocity);
            data->gamepad.a = report->a;
            data->gamepad.b = report->b;
            data->gamepad.x = report->x;
            data->gamepad.y = report->y;
            data->gamepad.dpadLeft = report->dpadLeft;
            data->gamepad.dpadRight = report->dpadRight;
            data->gamepad.dpadUp = report->dpadUp;
            data->gamepad.dpadDown = report->dpadDown;
            data->gamepad.back = report->back;
            data->gamepad.start = report->start;
            data->gamepad.guide = report->guide;
            break;
        }
        case XINPUT_GH_DRUMS: {
            XInputGuitarHeroDrums_Data_t *report = (XInputGuitarHeroDrums_Data_t *)data;
            data->gamepad.a = report->a;
            data->gamepad.b = report->b;
            data->gamepad.x = report->x;
            data->gamepad.y = report->y;
            data->gamepad.leftShoulder = report->leftShoulder;
            data->gamepad.back = report->back;
            data->gamepad.start = report->start;
            data->gamepad.guide = report->guide;
            data->gamepad.dpadLeft = report->dpadLeft;
            data->gamepad.dpadRight = report->dpadRight;
            data->gamepad.dpadUp = report->dpadUp;
            data->gamepad.dpadDown = report->dpadDown;
            data->midi.midiVelocities[GH_MIDI_NOTE_GREEN] = report->greenVelocity;
            data->midi.midiVelocities[GH_MIDI_NOTE_RED] = report->redVelocity;
            data->midi.midiVelocities[GH_MIDI_NOTE_YELLOW] = report->yellowVelocity;
            data->midi.midiVelocities[GH_MIDI_NOTE_BLUE] = report->blueVelocity;
            data->midi.midiVelocities[GH_MIDI_NOTE_ORANGE] = report->orangeVelocity;
            data->midi.midiVelocities[GH_MIDI_NOTE_KICK] = report->kickVelocity;
            // Forward any midi data we get
            memcpy(data->midi.midiPacket, report->midiPacket, sizeof(report->midiPacket));
            break;
        }
        case XINPUT_GUITAR_HERO_LIVE: {
            XInputGHLGuitar_Data_t *report = (XInputGHLGuitar_Data_t *)data;
            data->gamepad.a = report->a;
            data->gamepad.b = report->b;
            data->gamepad.x = report->x;
            data->gamepad.y = report->y;
            data->gamepad.leftShoulder = report->leftShoulder;
            data->gamepad.rightShoulder = report->rightShoulder;
            data->gamepad.dpadLeft = report->dpadLeft;
            data->gamepad.dpadRight = report->dpadRight;
            data->gamepad.dpadUp = report->dpadUp;
            data->gamepad.dpadDown = report->dpadDown;
            data->gamepad.leftThumbClick = report->leftThumbClick;
            data->gamepad.back = report->back;
            data->gamepad.start = report->start;
            data->gamepad.guide = report->guide;
            data->live_guitar.black1 = report->a;
            data->live_guitar.black2 = report->b;
            data->live_guitar.black3 = report->y;
            data->live_guitar.white1 = report->x;
            data->live_guitar.white2 = report->leftShoulder;
            data->live_guitar.white3 = report->rightShoulder;
            data->live_guitar.heroPower = report->back;
            data->live_guitar.ghtv = report->leftThumbClick;
            if (report->tilt) {
                data->live_guitar.tilt = report->tilt;
            }
            if (report->whammy) {
                data->live_guitar.whammy = report->whammy;
            }
            break;
        }
        case XINPUT_TURNTABLE: {
            XInputTurntable_Data_t *report = (XInputTurntable_Data_t *)data;
            data->gamepad.a = report->a;
            data->gamepad.b = report->b;
            data->gamepad.x = report->x;
            data->gamepad.y = report->y;
            data->gamepad.dpadLeft = report->dpadLeft;
            data->gamepad.dpadRight = report->dpadRight;
            data->gamepad.dpadUp = report->dpadUp;
            data->gamepad.dpadDown = report->dpadDown;
            data->gamepad.back = report->back;
            data->gamepad.start = report->start;
            data->gamepad.guide = report->guide;
            data->turntable.leftBlue = report->leftBlue;
            data->turntable.leftRed = report->leftRed;
            data->turntable.leftGreen = report->leftGreen;
            data->turntable.rightBlue = report->rightBlue;
            data->turntable.rightRed = report->rightRed;
            data->turntable.rightGreen = report->rightGreen;
            if (report->effectsKnob) {
                data->turntable.effectsKnob = report->effectsKnob;
            }
            if (report->crossfader) {
                data->turntable.crossfader = report->crossfader;
            }
            if (report->leftTableVelocity) {
                if (report->leftTableVelocity > 127) {
                    report->leftTableVelocity = 127;
                }
                if (report->leftTableVelocity < -127) {
                    report->leftTableVelocity = -127;
                }
                data->turntable.leftTableVelocity = report->leftTableVelocity << 8;
            }
            if (report->rightTableVelocity) {
                if (report->rightTableVelocity > 127) {
                    report->rightTableVelocity = 127;
                }
                if (report->rightTableVelocity < -127) {
                    report->rightTableVelocity = -127;
                }
                data->turntable.rightTableVelocity = report->rightTableVelocity << 8;
            }
            break;
        }
        case XINPUT_PRO_KEYS: {
            XInputRockBandKeyboard_Data_t *report = (XInputRockBandKeyboard_Data_t *)data;
            data->gamepad.dpadLeft = report->dpadLeft;
            data->gamepad.dpadRight = report->dpadRight;
            data->gamepad.dpadUp = report->dpadUp;
            data->gamepad.dpadDown = report->dpadDown;

            ProKeysParser::updateProKeys(report, data, ~report->pedalAnalog);
            break;
        }
        case XINPUT_PRO_GUITAR: {
            XInputRockBandProGuitar_Data_t *report = (XInputRockBandProGuitar_Data_t *)data;
            data->gamepad.dpadLeft = report->dpadLeft;
            data->gamepad.dpadRight = report->dpadRight;
            data->gamepad.dpadUp = report->dpadUp;
            data->gamepad.dpadDown = report->dpadDown;

            data->gamepad.a = report->a;
            data->gamepad.b = report->b;
            data->gamepad.x = report->x;
            data->gamepad.y = report->y;
            data->gamepad.back = report->back;
            data->gamepad.start = report->start;
            data->gamepad.guide = report->guide;
            ProGuitarParser::updateProGuitar(report, data);
            break;
        }
        // Any other subtypes we dont handle can just be read like gamepads.
        default: {
            XInputGamepad_Data_t *report = (XInputGamepad_Data_t *)data;
            data->gamepad.a = report->a;
            data->gamepad.b = report->b;
            data->gamepad.x = report->x;
            data->gamepad.y = report->y;
            data->gamepad.leftShoulder = report->leftShoulder;
            data->gamepad.rightShoulder = report->rightShoulder;
            data->gamepad.back = report->back;
            data->gamepad.start = report->start;
            data->gamepad.guide = report->guide;
            data->gamepad.leftThumbClick = report->leftThumbClick;
            data->gamepad.rightThumbClick = report->rightThumbClick;
            data->gamepad.dpadLeft = report->dpadLeft;
            data->gamepad.dpadRight = report->dpadRight;
            data->gamepad.dpadUp = report->dpadUp;
            data->gamepad.dpadDown = report->dpadDown;
            if (report->leftTrigger) {
                data->gamepad.leftTrigger = report->leftTrigger << 8;
            }
            if (report->rightTrigger) {
                data->gamepad.rightTrigger = report->rightTrigger << 8;
            }
            if (report->leftStickX) {
                data->gamepad.leftStickX = report->leftStickX;
            }
            if (report->leftStickY) {
                data->gamepad.leftStickY = report->leftStickY;
            }
            if (report->rightStickX) {
                data->gamepad.rightStickX = report->rightStickX;
            }
            if (report->rightStickY) {
                data->gamepad.rightStickY = report->rightStickY;
            }
            break;
        }
    }
}