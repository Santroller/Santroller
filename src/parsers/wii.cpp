#include "parsers/wii.hpp"
#include "parsers/drums.hpp"

void WiiParser::parse(uint8_t* wiiData, uint8_t len, san_base_t* data) {
    uint8_t wiiButtonsLow = ~wiiData[4];
    uint8_t wiiButtonsHigh = ~wiiData[5];
    if (hiRes) {
        wiiButtonsLow = ~wiiData[6];
        wiiButtonsHigh = ~wiiData[7];
    }
    switch (mType) {
        case WII_NUNCHUK:
            data->gamepad.accel[0] = ((wiiData[2] << 2) | ((wiiData[5] & 0xC0) >> 6)) - 511;
            data->gamepad.accel[1] = ((wiiData[3] << 2) | ((wiiData[5] & 0x30) >> 4)) - 511;
            data->gamepad.accel[2] = ((wiiData[4] << 2) | ((wiiData[5] & 0xC) >> 2)) - 511;
            data->gamepad.a = (wiiButtonsHigh) & (1 << 1);
            data->gamepad.b = (wiiButtonsHigh) & (1 << 0);
            data->gamepad.leftStickX = (wiiData[0] - 0x80) << 8;
            data->gamepad.leftStickY = (wiiData[1] - 0x80) << 8;
            break;
        case WII_CLASSIC_CONTROLLER:
        case WII_CLASSIC_CONTROLLER_PRO:
            if (hiRes) {
                data->gamepad.leftStickX = (wiiData[0] - 0x80) << 8;
                data->gamepad.leftStickY = (wiiData[2] - 0x80) << 8;
                data->gamepad.rightStickX = (wiiData[1] - 0x80) << 8;
                data->gamepad.rightStickY = (wiiData[3] - 0x80) << 8;
                data->gamepad.leftTrigger = wiiData[4] << 8;
                data->gamepad.rightTrigger = wiiData[5] << 8;
            } else {
                data->gamepad.leftStickX = ((wiiData[0] & 0x3f) - 32) << 10;
                data->gamepad.leftStickY = ((wiiData[1] & 0x3f) - 32) << 10;
                data->gamepad.rightStickX = ((((wiiData[0] & 0xc0) >> 3) | ((wiiData[1] & 0xc0) >> 5) | (wiiData[2] >> 7)) - 16) << 11;
                data->gamepad.rightStickY = ((wiiData[2] & 0x1f) - 16) << 11;
                data->gamepad.leftTrigger = (((wiiData[3] & 0xE0) >> 5 | (wiiData[2] & 0x60) >> 2)) << 11;
                data->gamepad.rightTrigger = (wiiData[3] & 0x1f) << 11;
            }
            data->gamepad.l2 = (wiiButtonsLow) & (1 << 5);
            data->gamepad.r2 = (wiiButtonsLow) & (1 << 1);
            data->gamepad.start = (wiiButtonsLow) & (1 << 2);
            data->gamepad.back = (wiiButtonsLow) & (1 << 4);
            data->gamepad.guide = (wiiButtonsLow) & (1 << 3);
            data->gamepad.dpadUp = (wiiButtonsHigh) & (1 << 0);
            data->gamepad.dpadDown = (wiiButtonsLow) & (1 << 6);
            data->gamepad.dpadLeft = (wiiButtonsHigh) & (1 << 1);
            data->gamepad.dpadRight = (wiiButtonsLow) & (1 << 7);
            data->gamepad.leftShoulder = (wiiButtonsHigh) & (1 << 7);
            data->gamepad.rightShoulder = (wiiButtonsHigh) & (1 << 2);

            // TODO: face button mapping swapping
            data->gamepad.a = (wiiButtonsHigh) & (1 << 4);
            data->gamepad.b = (wiiButtonsHigh) & (1 << 6);
            data->gamepad.x = (wiiButtonsHigh) & (1 << 3);
            data->gamepad.y = (wiiButtonsHigh) & (1 << 5);

            break;
        case WII_THQ_UDRAW_TABLET:
            data->mouse.x = ((wiiData[2] & 0x0f) << 8) | wiiData[0];
            data->mouse.y = ((wiiData[2] & 0xf0) << 4) | wiiData[1];
            data->mouse.penPressure = (wiiData[3]);
            data->mouse.left = (~wiiButtonsHigh) & (1 << 2);
            data->mouse.middle = (wiiButtonsHigh) & (1 << 0);
            data->mouse.right = (wiiButtonsHigh) & (1 << 1);
            break;
        case WII_UBISOFT_DRAWSOME_TABLET:
            data->mouse.x = (wiiData[0] | wiiData[1] << 8);
            data->mouse.y = (wiiData[2] | wiiData[3] << 8);
            data->mouse.penPressure = (wiiData[4] | (wiiData[5] & 0x0f) << 8);
            break;
        case WII_GUITAR_HERO_GUITAR_CONTROLLER: {
            data->gamepad.leftStickX = ((wiiData[0] & 0x3f) - 32) << 10;
            data->gamepad.leftStickY = ((wiiData[1] & 0x3f) - 32) << 10;
            data->gamepad.start = (wiiButtonsLow) & (1 << 2);
            data->gamepad.back = (wiiButtonsLow) & (1 << 4);
            data->guitar.whammy = (wiiData[3] & 0x1f) << 11;
            data->guitar.green = ((wiiButtonsHigh) & (1 << 4));
            data->guitar.red = ((wiiButtonsHigh) & (1 << 6));
            data->guitar.yellow = ((wiiButtonsHigh) & (1 << 3));
            data->guitar.blue = ((wiiButtonsHigh) & (1 << 5));
            data->guitar.orange = ((wiiButtonsHigh) & (1 << 7));
            data->guitar.pedal = ((wiiButtonsHigh) & (1 << 2));
            data->guitar.strumUp = ((wiiButtonsHigh) & (1 << 0));
            data->guitar.strumDown = ((wiiButtonsLow) & (1 << 6));
            uint8_t lastTapWii = (wiiData[2] & 0x1f);
            // convert GH tap bar to solo frets
            if (!hasTapBar) {
                // only guitars with a tap bar will ever set this to 0x0F
                if (lastTapWii == 0x0F) {
                    hasTapBar = true;
                }
            } else if (lastTapWii < 0x05) {
                data->guitar.soloGreen = true;
                data->guitar.soloRed = false;
                data->guitar.soloYellow = false;
                data->guitar.soloBlue = false;
                data->guitar.soloOrange = false;
            } else if (lastTapWii < 0x0A) {
                data->guitar.soloGreen = true;
                data->guitar.soloRed = true;
                data->guitar.soloYellow = false;
                data->guitar.soloBlue = false;
                data->guitar.soloOrange = false;
            } else if (lastTapWii < 0x0C) {
                data->guitar.soloGreen = false;
                data->guitar.soloRed = true;
                data->guitar.soloYellow = false;
                data->guitar.soloBlue = false;
                data->guitar.soloOrange = false;
            } else if (lastTapWii == 0x0f) {
                data->guitar.soloGreen = false;
                data->guitar.soloRed = false;
                data->guitar.soloYellow = false;
                data->guitar.soloBlue = false;
                data->guitar.soloOrange = false;
            } else if (lastTapWii < 0x12) {
                data->guitar.soloGreen = false;
                data->guitar.soloRed = true;
                data->guitar.soloYellow = true;
                data->guitar.soloBlue = false;
                data->guitar.soloOrange = false;
            } else if (lastTapWii < 0x14) {
                data->guitar.soloGreen = false;
                data->guitar.soloRed = false;
                data->guitar.soloYellow = true;
                data->guitar.soloBlue = false;
                data->guitar.soloOrange = false;
            } else if (lastTapWii < 0x17) {
                data->guitar.soloGreen = false;
                data->guitar.soloRed = false;
                data->guitar.soloYellow = true;
                data->guitar.soloBlue = true;
                data->guitar.soloOrange = false;
            } else if (lastTapWii < 0x1A) {
                data->guitar.soloGreen = false;
                data->guitar.soloRed = false;
                data->guitar.soloYellow = false;
                data->guitar.soloBlue = true;
                data->guitar.soloOrange = false;
            } else if (lastTapWii < 0x1F) {
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
            break;
        }
        case WII_GUITAR_HERO_DRUM_CONTROLLER: {
            data->gamepad.leftStickX = ((wiiData[0] & 0x3f) - 32) << 10;
            data->gamepad.leftStickY = ((wiiData[1] & 0x3f) - 32) << 10;
            data->gamepad.start = (wiiButtonsLow) & (1 << 2);
            data->gamepad.back = (wiiButtonsLow) & (1 << 4);
            // GH drums send us raw midi, so we just ignore all the buttons and deal with the MIDI data as-is
            uint8_t velocity = ((wiiData[4] & 0b00000001) |
                                ((wiiData[4] & 0b10000000) >> 6) |
                                ((wiiData[3] & 0b00000001) << 2) |
                                ((wiiData[2] & 0b00000001) << 3) |
                                ((wiiData[3] & (0b11100000)) >> 1));
            uint8_t note = (wiiData[2] >> 1) & 0x7f;
            uint8_t channel = ((~wiiData[3]) >> 1) & 0xF;
            velocity = 0x7F - velocity;
            note = 0x7F - note;
            if (velocity || note) {
                data->midi.midiVelocities[note] = velocity;
            }
            
            if (!wiiButtonsHigh & (1 << 4)) {
                data->midi.midiVelocities[GH_MIDI_NOTE_GREEN] = 0;
            }
            if (!wiiButtonsHigh & (1 << 6)) {
                data->midi.midiVelocities[GH_MIDI_NOTE_RED] = 0;
            }
            if (!wiiButtonsHigh & (1 << 5)) {
                data->midi.midiVelocities[GH_MIDI_NOTE_YELLOW] = 0;
            }
            if (!wiiButtonsHigh & (1 << 3)) {
                data->midi.midiVelocities[GH_MIDI_NOTE_BLUE] = 0;
            }
            if (!wiiButtonsHigh & (1 << 7)) {
                data->midi.midiVelocities[GH_MIDI_NOTE_ORANGE] = 0;
            }
            break;
        }
        case WII_DJ_HERO_TURNTABLE: {
            struct {
                union {
                    signed int ltt : 6;
                    struct {
                        unsigned int ltt40 : 5;
                        unsigned int ltt5 : 1;
                    };
                };
            } ltt_t;
            struct {
                union {
                    signed int rtt : 6;
                    struct {
                        unsigned int rtt0 : 1;
                        unsigned int rtt21 : 2;
                        unsigned int rtt43 : 2;
                        unsigned int rtt5 : 1;
                    };
                };
            } rtt_t;
            ltt_t.ltt5 = (wiiData[4] & 1);
            ltt_t.ltt40 = (wiiData[3] & 0x1F);
            rtt_t.rtt0 = (wiiData[2] & 0x80) >> 7;
            rtt_t.rtt21 = (wiiData[1] & 0xC0) >> 6;
            rtt_t.rtt43 = (wiiData[0] & 0xC0) >> 6;
            rtt_t.rtt5 = (wiiData[2] & 1);
            data->gamepad.leftStickX = ((wiiData[0] & 0x3f) - 32) << 10;
            data->gamepad.leftStickY = ((wiiData[1] & 0x3f) - 32) << 10;
            data->gamepad.start = (wiiButtonsLow) & (1 << 2);
            data->gamepad.back = (wiiButtonsLow) & (1 << 4);
            data->turntable.leftTableVelocity = (ltt_t.ltt << 10);
            data->turntable.rightTableVelocity = (rtt_t.rtt << 10);
            data->turntable.crossfader = ((wiiData[2] & 0x1E) >> 1) << 12;
            data->turntable.effectsKnob = (((wiiData[3] & 0xE0) >> 5 | (wiiData[2] & 0x60) >> 2)) << 11;
            data->turntable.leftGreen = (wiiButtonsHigh) & (1 << 3);
            data->turntable.leftRed = (wiiButtonsLow) & (1 << 5);
            data->turntable.leftBlue = (wiiButtonsHigh) & (1 << 7);
            data->turntable.rightGreen = (wiiButtonsHigh) & (1 << 5);
            data->turntable.rightRed = (wiiButtonsLow) & (1 << 1);
            data->turntable.rightBlue = (wiiButtonsHigh) & (1 << 2);
            data->turntable.euphoria = (wiiButtonsHigh) & (1 << 4);
            break;
        }
        case WII_TAIKO_NO_TATSUJIN_CONTROLLER:
            data->taiko.leftDrumRim = (~wiiData[0]) & (1 << 5);
            data->taiko.leftDrumCenter = (~wiiData[0]) & (1 << 6);
            data->taiko.rightDrumRim = (~wiiData[0]) & (1 << 3);
            data->taiko.rightDrumCenter = (~wiiData[0]) & (1 << 4);
            break;
    }
}