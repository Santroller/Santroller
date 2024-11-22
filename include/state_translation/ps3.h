#include <stdint.h>

#include "defines.h"
#include "reports/controller_reports.h"
#include "state_translation/shared.h"
extern bool hasFlags;
extern const uint8_t dpad_bindings[11];
extern const uint8_t dpad_bindings_reverse[8];
inline void ps3_to_universal_report(const uint8_t *data, uint8_t len, uint8_t sub_type, USB_Host_Data_t *usb_host_data) {
    if (sub_type != GAMEPAD) {
        PS3Dpad_Data_t *report = (PS3Dpad_Data_t *)data;
        DPAD_REV();
    }
    switch (sub_type) {
        case GAMEPAD: {
            PS3Gamepad_Data_t *report = (PS3Gamepad_Data_t *)data;
            usb_host_data->a |= report->a;
            usb_host_data->b |= report->b;
            usb_host_data->x |= report->x;
            usb_host_data->y |= report->y;
            usb_host_data->leftShoulder |= report->leftShoulder;
            usb_host_data->rightShoulder |= report->rightShoulder;
            if (report->leftTrigger) {
                usb_host_data->leftTrigger = report->leftTrigger << 8;
            }
            if (report->rightTrigger) {
                usb_host_data->rightTrigger = report->rightTrigger << 8;
            }
            if (report->leftStickX != 0x80) {
                usb_host_data->leftStickX = (report->leftStickX - 0x80) << 8;
            }
            if (report->leftStickY != 0x80) {
                usb_host_data->leftStickY = (-report->leftStickY - 0x80) << 8;
            }
            if (report->rightStickX != 0x80) {
                usb_host_data->rightStickX = (report->rightStickX - 0x80) << 8;
            }
            if (report->rightStickY != 0x80) {
                usb_host_data->rightStickY = (-report->rightStickY - 0x80) << 8;
            }
            usb_host_data->leftThumbClick |= report->leftThumbClick;
            usb_host_data->rightThumbClick |= report->rightThumbClick;
            if (report->pressureDpadUp) {
                usb_host_data->pressureDpadUp = report->pressureDpadUp;
            }
            if (report->pressureDpadRight) {
                usb_host_data->pressureDpadRight = report->pressureDpadRight;
            }
            if (report->pressureDpadDown) {
                usb_host_data->pressureDpadDown = report->pressureDpadDown;
            }
            if (report->pressureDpadLeft) {
                usb_host_data->pressureDpadLeft = report->pressureDpadLeft;
            }
            if (report->pressureL1) {
                usb_host_data->pressureL1 = report->pressureL1;
            }
            if (report->pressureR1) {
                usb_host_data->pressureR1 = report->pressureR1;
            }
            if (report->pressureTriangle) {
                usb_host_data->pressureTriangle = report->pressureTriangle;
            }
            if (report->pressureCircle) {
                usb_host_data->pressureCircle = report->pressureCircle;
            }
            if (report->pressureCross) {
                usb_host_data->pressureCross = report->pressureCross;
            }
            if (report->pressureSquare) {
                usb_host_data->pressureSquare = report->pressureSquare;
            };
            break;
        }
        case ROCK_BAND_PRO_KEYS: {
            PS3RockBandProKeyboard_Data_t *report = (PS3RockBandProKeyboard_Data_t *)data;
            usb_host_data->a |= report->a;
            usb_host_data->b |= report->b;
            usb_host_data->x |= report->x;
            usb_host_data->y |= report->y;
            usb_host_data->key1 |= report->key1;
            usb_host_data->key2 |= report->key2;
            usb_host_data->key3 |= report->key3;
            usb_host_data->key4 |= report->key4;
            usb_host_data->key5 |= report->key5;
            usb_host_data->key6 |= report->key6;
            usb_host_data->key7 |= report->key7;
            usb_host_data->key8 |= report->key8;
            usb_host_data->key9 |= report->key9;
            usb_host_data->key10 |= report->key10;
            usb_host_data->key11 |= report->key11;
            usb_host_data->key12 |= report->key12;
            usb_host_data->key13 |= report->key13;
            usb_host_data->key14 |= report->key14;
            usb_host_data->key15 |= report->key15;
            usb_host_data->key16 |= report->key16;
            usb_host_data->key17 |= report->key17;
            usb_host_data->key18 |= report->key18;
            usb_host_data->key19 |= report->key19;
            usb_host_data->key20 |= report->key20;
            usb_host_data->key21 |= report->key21;
            usb_host_data->key22 |= report->key22;
            usb_host_data->key23 |= report->key23;
            usb_host_data->key24 |= report->key24;
            usb_host_data->key25 |= report->key25;
            usb_host_data->overdrive |= report->overdrive;
            usb_host_data->kick1 |= report->kick1;
            if (report->kickVelocity) {
                usb_host_data->kickVelocity = report->kickVelocity << 1;
            }
            if (report->touchPad) {
                usb_host_data->touchPad = report->touchPad << 1;
            }
            if (report->velocities[0]) {
                usb_host_data->velocities[0] = report->velocities[0];
            }
            if (report->velocities[1]) {
                usb_host_data->velocities[1] = report->velocities[1];
            }
            if (report->velocities[2]) {
                usb_host_data->velocities[2] = report->velocities[2];
            }
            if (report->velocities[3]) {
                usb_host_data->velocities[3] = report->velocities[3];
            }
            if (report->velocities[4]) {
                usb_host_data->velocities[4] = report->velocities[4];
            };
            break;
        }
        case ROCK_BAND_GUITAR: {
            PS3RockBandGuitar_Data_t *report = (PS3RockBandGuitar_Data_t *)data;
            usb_host_data->green |= report->green;
            usb_host_data->red |= report->red;
            usb_host_data->yellow |= report->yellow;
            usb_host_data->blue |= report->blue;
            usb_host_data->orange |= report->orange;
            if (report->tilt) {
                usb_host_data->tilt = report->tilt ? 0x7fff : 0;
            }
            if (report->whammy) {
                usb_host_data->whammy = (report->whammy - 0x80) << 8;
            }
            if (report->leftStickX) {
                usb_host_data->leftStickX = (report->leftStickX - 0x80) << 8;
            }
            if (report->leftStickY) {
                usb_host_data->leftStickY = (-report->leftStickY - 0x80) << 8;
            }
            if (report->pickup) {
                usb_host_data->pickup = report->pickup;
            };
            if (report->solo) {
                usb_host_data->soloGreen |= report->green;
                usb_host_data->soloRed |= report->red;
                usb_host_data->soloYellow |= report->yellow;
                usb_host_data->soloBlue |= report->blue;
                usb_host_data->soloOrange |= report->orange;
            };
            break;
        }
        case GUITAR_HERO_GUITAR_WT:
        case GUITAR_HERO_GUITAR: {
            PS3GuitarHeroGuitar_Data_t *report = (PS3GuitarHeroGuitar_Data_t *)data;
            if (sub_type == 13) {
                if (report->slider < 0x2F) {
                    usb_host_data->slider = 0x15;
                } else if (report->slider <= 0x3F) {
                    usb_host_data->slider = 0x30;
                } else if (report->slider <= 0x5F) {
                    usb_host_data->slider = 0x4D;
                } else if (report->slider <= 0x6F) {
                    usb_host_data->slider = 0x66;
                } else if (report->slider <= 0x8F) {
                    usb_host_data->slider = 0x80;
                } else if (report->slider <= 0x9F) {
                    usb_host_data->slider = 0x9A;
                } else if (report->slider <= 0xAF) {
                    usb_host_data->slider = 0xAF;
                } else if (report->slider <= 0xCF) {
                    usb_host_data->slider = 0xC9;
                } else if (report->slider <= 0xEF) {
                    usb_host_data->slider = 0xE6;
                } else {
                    usb_host_data->slider = 0xFF;
                }
            };
            report->green |= usb_host_data->green;
            report->red |= usb_host_data->red;
            report->yellow |= usb_host_data->yellow;
            report->blue |= usb_host_data->blue;
            report->orange |= usb_host_data->orange;
            report->kick1 |= usb_host_data->kick1;
            if (usb_host_data->tilt) {
                report->tilt = (-((usb_host_data->tilt - 0x0200) + 40)) << 7;
            }
            if (usb_host_data->whammy) {
                report->whammy = (usb_host_data->whammy >> 1) + 0x80;
            }
            if (usb_host_data->leftStickX) {
                report->leftStickX = usb_host_data->leftStickX;
            }
            if (usb_host_data->leftStickY) {
                report->leftStickY = -usb_host_data->leftStickY;
            }
            if (usb_host_data->slider) {
                report->slider = usb_host_data->slider;
            };
            break;
        }
        case ROCK_BAND_DRUMS: {
            PS3RockBandDrums_Data_t *report = (PS3RockBandDrums_Data_t *)data;
            bool green = report->green;
            bool red = report->red;
            bool yellow = report->yellow;
            bool blue = report->blue;
            bool pad = report->padFlag;
            bool cymbal = report->cymbalFlag;
            if (pad || cymbal) {
                hasFlags = true;
            }
            if (pad && cymbal) {
                int colorCount = 0;
                colorCount += red ? 1 : 0;
                colorCount += (yellow || report->dpadUp) ? 1 : 0;
                colorCount += (blue || report->dpadDown) ? 1 : 0;
                colorCount += (green || !(report->dpadUp || report->dpadDown)) ? 1 : 0;
                if (colorCount > 1) {
                    if (report->dpadUp) {
                        usb_host_data->yellowCymbal = true;
                        yellow = false;
                        cymbal = false;
                    }
                    if (report->dpadDown) {
                        usb_host_data->blueCymbal = true;
                        blue = false;
                        cymbal = false;
                    }
                    if (!(report->dpadUp || report->dpadDown)) {
                        usb_host_data->greenCymbal = true;
                        green = false;
                        cymbal = false;
                    }
                }
            }
            if (pad || (!cymbal && !hasFlags)) {
                usb_host_data->red |= red;
                usb_host_data->green |= green;
                usb_host_data->yellow |= yellow;
                usb_host_data->blue |= blue;
            }
            if (cymbal) {
                usb_host_data->greenCymbal |= green;
                usb_host_data->blueCymbal |= blue;
                usb_host_data->yellowCymbal |= yellow;
            };
            usb_host_data->green |= report->green;
            usb_host_data->red |= report->red;
            usb_host_data->yellow |= report->yellow;
            usb_host_data->blue |= report->blue;
            usb_host_data->kick1 |= report->kick1;
            usb_host_data->kick2 |= report->kick2;
            if (report->greenVelocity) {
                if (usb_host_data->greenCymbal) {
                    usb_host_data->greenCymbalVelocity = 0xFF - report->greenVelocity;
                } else {
                    usb_host_data->greenVelocity = 0xFF - report->greenVelocity;
                }
            }
            if (report->redVelocity) {
                usb_host_data->redVelocity = 0xFF - report->redVelocity;
            }
            if (report->yellowVelocity) {
                if (usb_host_data->yellowCymbal) {
                    usb_host_data->yellowCymbalVelocity = 0xFF - report->yellowVelocity;
                } else {
                    usb_host_data->yellowVelocity = 0xFF - report->yellowVelocity;
                }
            }
            if (report->blueVelocity) {
                if (usb_host_data->blueCymbal) {
                    usb_host_data->blueCymbalVelocity = 0xFF - report->blueVelocity;
                } else {
                    usb_host_data->blueVelocity = 0xFF - report->blueVelocity;
                }
            };
            break;
        }
        case GUITAR_HERO_DRUMS: {
            PS3GuitarHeroDrums_Data_t *report = (PS3GuitarHeroDrums_Data_t *)data;
            usb_host_data->green |= report->green;
            usb_host_data->red |= report->red;
            usb_host_data->yellow |= report->yellow;
            usb_host_data->blue |= report->blue;
            usb_host_data->orange |= report->orange;
            usb_host_data->kick1 |= report->kick1;
            if (report->greenVelocity) {
                usb_host_data->greenVelocity = report->greenVelocity;
            }
            if (report->redVelocity) {
                usb_host_data->redVelocity = report->redVelocity;
            }
            if (report->yellowVelocity) {
                usb_host_data->yellowVelocity = report->yellowVelocity;
            }
            if (report->blueVelocity) {
                usb_host_data->blueVelocity = report->blueVelocity;
            }
            if (report->orangeVelocity) {
                usb_host_data->orangeVelocity = report->orangeVelocity;
            }
            if (report->kickVelocity) {
                usb_host_data->kickVelocity = report->kickVelocity;
            };
            break;
        }
        case LIVE_GUITAR: {
            PS3GHLGuitar_Data_t *report = (PS3GHLGuitar_Data_t *)data;
            usb_host_data->black1 |= report->black1;
            usb_host_data->black2 |= report->black2;
            usb_host_data->black3 |= report->black3;
            usb_host_data->white1 |= report->white1;
            usb_host_data->white2 |= report->white2;
            usb_host_data->white3 |= report->white3;
            usb_host_data->ghtv |= report->ghtv;
            if (report->dpadUp) {
                usb_host_data->leftStickY = 0x00;
            }
            if (report->dpadDown) {
                usb_host_data->leftStickY = 0xFF;
            }
            if (report->tilt) {
                usb_host_data->tilt = (-((report->tilt - 0x0200) + 40)) << 7;
            }
            if (report->whammy) {
                usb_host_data->whammy = report->whammy;
            };
            break;
        }
        case DJ_HERO_TURNTABLE: {
            PS3Turntable_Data_t *report = (PS3Turntable_Data_t *)data;
            usb_host_data->a |= report->a;
            usb_host_data->b |= report->b;
            usb_host_data->x |= report->x;
            usb_host_data->leftBlue |= report->leftBlue;
            usb_host_data->leftRed |= report->leftRed;
            usb_host_data->leftGreen |= report->leftGreen;
            usb_host_data->rightBlue |= report->rightBlue;
            usb_host_data->rightRed |= report->rightRed;
            usb_host_data->rightGreen |= report->rightGreen;
            usb_host_data->euphoria |= report->euphoria;
            if (report->effectsKnob) {
                usb_host_data->effectsKnob = (report->effectsKnob - 0x0200) << 6;
            }
            if (report->crossfader) {
                usb_host_data->crossfader = (report->crossfader - 0x0200) << 6;
            }
            if (report->leftTableVelocity) {
                usb_host_data->leftTableVelocity = (report->leftTableVelocity - 0x80) << 8;
            }
            if (report->rightTableVelocity) {
                usb_host_data->rightTableVelocity = (report->rightTableVelocity - 0x80) << 8;
            };
            break;
        }

        case ROCK_BAND_PRO_GUITAR_SQUIRE:
        case ROCK_BAND_PRO_GUITAR_MUSTANG: {
            PS3RockBandProGuitar_Data_t *report = (PS3RockBandProGuitar_Data_t *)data;
            usb_host_data->lowEFretVelocity = report->lowEFretVelocity;
            usb_host_data->aFretVelocity = report->aFretVelocity;
            usb_host_data->dFretVelocity = report->dFretVelocity;
            usb_host_data->gFretVelocity = report->gFretVelocity;
            usb_host_data->bFretVelocity = report->bFretVelocity;
            usb_host_data->highEFretVelocity = report->highEFretVelocity;
            usb_host_data->a |= report->a;
            usb_host_data->b |= report->b;
            usb_host_data->x |= report->x;
            usb_host_data->y |= report->y;
            usb_host_data->green |= report->green;
            usb_host_data->red |= report->red;
            usb_host_data->yellow |= report->yellow;
            usb_host_data->blue |= report->blue;
            usb_host_data->orange |= report->orange;
            usb_host_data->lowEFret = report->lowEFret;
            usb_host_data->aFret = report->aFret;
            usb_host_data->dFret = report->dFret;
            usb_host_data->gFret = report->gFret;
            usb_host_data->bFret = report->bFret;
            usb_host_data->highEFret = report->highEFret;
            usb_host_data->tilt |= report->tilt;
            usb_host_data->kick1 |= report->kick1;
            break;
        }
    }
}

inline uint8_t universal_report_to_ps3(uint8_t dpad, uint8_t *data, uint8_t console_type, uint8_t sub_type, const USB_Host_Data_t *usb_host_data) {
    if (sub_type != GAMEPAD) {
        PS3Dpad_Data_t *report = (PS3Dpad_Data_t *)data;
        report->dpad = dpad;
        report->guide = usb_host_data->guide;
        report->back = usb_host_data->back;
        report->start = usb_host_data->start;
    }
    switch (sub_type) {
        case GAMEPAD: {
            PS3Gamepad_Data_t *report = (PS3Gamepad_Data_t *)data;
            report->dpadLeft = usb_host_data->dpadLeft;
            report->dpadRight = usb_host_data->dpadRight;
            report->dpadUp = usb_host_data->dpadUp;
            report->dpadDown = usb_host_data->dpadDown;
            report->a = usb_host_data->a;
            report->b = usb_host_data->b;
            report->x = usb_host_data->x;
            report->y = usb_host_data->y;
            report->leftShoulder = usb_host_data->leftShoulder;
            report->rightShoulder = usb_host_data->rightShoulder;
            report->leftTrigger = usb_host_data->leftTrigger >> 8;
            report->rightTrigger = usb_host_data->rightTrigger >> 8;
            report->leftStickX = (usb_host_data->leftStickX >> 8) + 0x80;
            report->leftStickY = (-usb_host_data->leftStickY >> 8) + 0x80;
            report->rightStickX = (usb_host_data->rightStickX >> 8) + 0x80;
            report->rightStickY = (-usb_host_data->rightStickY >> 8) + 0x80;
            report->leftThumbClick = usb_host_data->leftThumbClick;
            report->rightThumbClick = usb_host_data->rightThumbClick;
            report->pressureDpadUp = usb_host_data->pressureDpadUp;
            report->pressureDpadRight = usb_host_data->pressureDpadRight;
            report->pressureDpadDown = usb_host_data->pressureDpadDown;
            report->pressureDpadLeft = usb_host_data->pressureDpadLeft;
            report->pressureL1 = usb_host_data->pressureL1;
            report->pressureR1 = usb_host_data->pressureR1;
            report->pressureTriangle = usb_host_data->pressureTriangle;
            report->pressureCircle = usb_host_data->pressureCircle;
            report->pressureCross = usb_host_data->pressureCross;
            report->pressureSquare = usb_host_data->pressureSquare;
            if (SWAP_SWITCH_FACE_BUTTONS && console_type == SWITCH) {
                bool a = report->a;
                bool b = report->b;
                bool x = report->x;
                bool y = report->y;
                report->b = a;
                report->a = b;
                report->x = y;
                report->y = x;
            }
            if (console_type == IOS_FESTIVAL) {
                report->accelX = PS3_ACCEL_CENTER;
                report->accelY = PS3_ACCEL_CENTER;
                report->accelZ = PS3_ACCEL_CENTER;
                report->gyro = PS3_ACCEL_CENTER;
                report->leftStickX = PS3_STICK_CENTER;
                report->leftStickY = PS3_STICK_CENTER;
                report->rightStickX = PS3_STICK_CENTER;
                report->rightStickY = PS3_STICK_CENTER;
            }
            return sizeof(PS3Gamepad_Data_t);
        }
        case ROCK_BAND_PRO_KEYS: {
            PS3RockBandProKeyboard_Data_t *report = (PS3RockBandProKeyboard_Data_t *)data;
            report->a = usb_host_data->a;
            report->b = usb_host_data->b;
            report->x = usb_host_data->x;
            report->y = usb_host_data->y;
            report->key1 = usb_host_data->key1;
            report->key2 = usb_host_data->key2;
            report->key3 = usb_host_data->key3;
            report->key4 = usb_host_data->key4;
            report->key5 = usb_host_data->key5;
            report->key6 = usb_host_data->key6;
            report->key7 = usb_host_data->key7;
            report->key8 = usb_host_data->key8;
            report->key9 = usb_host_data->key9;
            report->key10 = usb_host_data->key10;
            report->key11 = usb_host_data->key11;
            report->key12 = usb_host_data->key12;
            report->key13 = usb_host_data->key13;
            report->key14 = usb_host_data->key14;
            report->key15 = usb_host_data->key15;
            report->key16 = usb_host_data->key16;
            report->key17 = usb_host_data->key17;
            report->key18 = usb_host_data->key18;
            report->key19 = usb_host_data->key19;
            report->key20 = usb_host_data->key20;
            report->key21 = usb_host_data->key21;
            report->key22 = usb_host_data->key22;
            report->key23 = usb_host_data->key23;
            report->key24 = usb_host_data->key24;
            report->key25 = usb_host_data->key25;
            report->overdrive = usb_host_data->overdrive;
            report->kick1 = usb_host_data->kick1;
            report->kickVelocity = usb_host_data->kickVelocity << 1;
            report->touchPad = usb_host_data->touchPad << 1;
            report->velocities[0] = usb_host_data->velocities[0];
            report->velocities[1] = usb_host_data->velocities[1];
            report->velocities[2] = usb_host_data->velocities[2];
            report->velocities[3] = usb_host_data->velocities[3];
            report->velocities[4] = usb_host_data->velocities[4];
            return sizeof(PS3RockBandProKeyboard_Data_t);
        }
        case ROCK_BAND_GUITAR: {
            PS3RockBandGuitar_Data_t *report = (PS3RockBandGuitar_Data_t *)data;
            report->green = usb_host_data->green;
            report->red = usb_host_data->red;
            report->yellow = usb_host_data->yellow;
            report->blue = usb_host_data->blue;
            report->orange = usb_host_data->orange;
            report->tilt = usb_host_data->tilt > 20000;
            report->whammy = (usb_host_data->whammy >> 8) + 0x80;
            report->leftStickX = (usb_host_data->leftStickX >> 8) + 0x80;
            report->leftStickY = (-usb_host_data->leftStickY >> 8) + 0x80;
            report->pickup = usb_host_data->pickup;
            report->green = usb_host_data->soloGreen;
            report->red = usb_host_data->soloRed;
            report->yellow = usb_host_data->soloYellow;
            report->blue = usb_host_data->soloBlue;
            report->orange = usb_host_data->soloOrange;
            report->solo = usb_host_data->soloGreen || usb_host_data->soloRed || usb_host_data->soloYellow || usb_host_data->soloBlue || usb_host_data->soloOrange;
            return sizeof(PS3RockBandGuitar_Data_t);
        }
        case FORTNITE_GUITAR: {
            if (console_type == SWITCH) {
                SwitchFestivalProGuitarLayer_Data_t *report = (SwitchFestivalProGuitarLayer_Data_t *)data;
                report->dpad = 0;
                report->green = usb_host_data->green;
                report->red = usb_host_data->red;
                report->yellow = usb_host_data->yellow;
                report->blue = usb_host_data->blue;
                report->orange = usb_host_data->orange;
                report->tilt = usb_host_data->tilt > 20000;
                report->whammy = usb_host_data->whammy > 0x70;
                report->green = usb_host_data->soloGreen;
                report->red = usb_host_data->soloRed;
                report->yellow = usb_host_data->soloYellow;
                report->blue = usb_host_data->soloBlue;
                report->orange = usb_host_data->soloOrange;
                report->dpad = dpad_bindings[report->dpad];
                return sizeof(SwitchFestivalProGuitarLayer_Data_t);
            } else if (console_type == IOS_FESTIVAL) {
                PS3FestivalProGuitarLayer_Data_t *report = (PS3FestivalProGuitarLayer_Data_t *)data;
                report->reportId = 0x01;
                report->green = usb_host_data->green;
                report->red = usb_host_data->red;
                report->yellow = usb_host_data->yellow;
                report->blue = usb_host_data->blue;
                report->orange = usb_host_data->orange;
                report->tilt = usb_host_data->tilt > 20000;
                report->whammy = usb_host_data->whammy > 0x70;
                report->green = usb_host_data->soloGreen;
                report->red = usb_host_data->soloRed;
                report->yellow = usb_host_data->soloYellow;
                report->blue = usb_host_data->soloBlue;
                report->orange = usb_host_data->soloOrange;
                return sizeof(PS3FestivalProGuitarLayer_Data_t);
            }
        }
        case GUITAR_HERO_GUITAR_WT:
        case GUITAR_HERO_GUITAR: {
            PS3GuitarHeroGuitar_Data_t *report = (PS3GuitarHeroGuitar_Data_t *)data;
            report->green = usb_host_data->green;
            report->red = usb_host_data->red;
            report->yellow = usb_host_data->yellow;
            report->blue = usb_host_data->blue;
            report->orange = usb_host_data->orange;
            report->kick1 = usb_host_data->kick1;
            report->tilt = 0x0200 + ((-(usb_host_data->tilt >> 7)) - 40);
            report->whammy = (usb_host_data->whammy >> 1) + 0x80;
            report->leftStickX = (usb_host_data->leftStickX >> 8) + 0x80;
            report->leftStickY = (-usb_host_data->leftStickY >> 8) + 0x80;
            report->slider = usb_host_data->slider;
            return sizeof(PS3GuitarHeroGuitar_Data_t);
        }
        case GUITAR_HERO_GUITAR_PS2_ON_PS3: {
            PS2OnPs3GuitarHeroGuitar_Data_t *report = (PS2OnPs3GuitarHeroGuitar_Data_t *)data;
            report->green = usb_host_data->green;
            report->red = usb_host_data->red;
            report->yellow = usb_host_data->yellow;
            report->blue = usb_host_data->blue;
            report->orange = usb_host_data->orange;
            report->tilt = usb_host_data->tilt > 20000;
            report->whammy = 0x7f - (usb_host_data->whammy >> 1);
            return sizeof(PS2OnPs3GuitarHeroGuitar_Data_t);
        }
        case ROCK_BAND_DRUMS: {
            PS3RockBandDrums_Data_t *report = (PS3RockBandDrums_Data_t *)data;
            report->green = usb_host_data->green;
            report->red = usb_host_data->red;
            report->yellow = usb_host_data->yellow;
            report->blue = usb_host_data->blue;
            report->kick1 = usb_host_data->kick1;
            report->kick2 = usb_host_data->kick2;
            report->padFlag = usb_host_data->green || usb_host_data->red || usb_host_data->yellow || usb_host_data->blue;
            report->cymbalFlag = usb_host_data->greenCymbal || usb_host_data->yellowCymbal || usb_host_data->blueCymbal;
            if (usb_host_data->greenVelocity) {
                if (usb_host_data->greenCymbalVelocity) {
                    report->greenVelocity = -((0x7FFF - (usb_host_data->greenCymbalVelocity << 7)));
                } else {
                    report->greenVelocity = -((0x7FFF - (usb_host_data->greenVelocity << 7)));
                }
            }
            if (usb_host_data->redVelocity) {
                report->redVelocity = (0x7FFF - (usb_host_data->redVelocity << 7));
            }
            if (usb_host_data->yellowVelocity) {
                if (usb_host_data->yellowCymbalVelocity) {
                    report->yellowVelocity = -((0x7FFF - (usb_host_data->yellowCymbalVelocity << 7)));
                } else {
                    report->yellowVelocity = -((0x7FFF - (usb_host_data->yellowVelocity << 7)));
                }
            }
            if (usb_host_data->blueVelocity) {
                if (usb_host_data->blueCymbalVelocity) {
                    report->blueVelocity = (0x7FFF - (usb_host_data->blueCymbalVelocity << 7));
                } else {
                    report->blueVelocity = (0x7FFF - (usb_host_data->blueVelocity << 7));
                }
            };
            return sizeof(PS3RockBandDrums_Data_t);
        }
        case GUITAR_HERO_DRUMS: {
            PS3GuitarHeroDrums_Data_t *report = (PS3GuitarHeroDrums_Data_t *)data;
            report->green = usb_host_data->green;
            report->red = usb_host_data->red;
            report->yellow = usb_host_data->yellow;
            report->blue = usb_host_data->blue;
            report->orange = usb_host_data->orange;
            report->kick1 = usb_host_data->kick1;
            report->greenVelocity = usb_host_data->greenVelocity;
            report->redVelocity = usb_host_data->redVelocity;
            report->yellowVelocity = usb_host_data->yellowVelocity;
            report->blueVelocity = usb_host_data->blueVelocity;
            report->orangeVelocity = usb_host_data->orangeVelocity;
            report->kickVelocity = usb_host_data->kickVelocity;
            return sizeof(PS3GuitarHeroDrums_Data_t);
        }
        case LIVE_GUITAR: {
            PS3GHLGuitar_Data_t *report = (PS3GHLGuitar_Data_t *)data;
            report->black1 = usb_host_data->black1;
            report->black2 = usb_host_data->black2;
            report->black3 = usb_host_data->black3;
            report->white1 = usb_host_data->white1;
            report->white2 = usb_host_data->white2;
            report->white3 = usb_host_data->white3;
            report->ghtv = usb_host_data->ghtv;
            if (usb_host_data->dpadUp) {
                report->leftStickY = 0x00;
            }
            if (usb_host_data->dpadDown) {
                report->leftStickY = 0xFF;
            }
            report->tilt = 0x0200 + ((-(usb_host_data->tilt >> 7)) - 40);
            report->whammy = usb_host_data->whammy;
            return sizeof(PS3GHLGuitar_Data_t);
        }
        case DJ_HERO_TURNTABLE: {
            PS3Turntable_Data_t *report = (PS3Turntable_Data_t *)data;
            report->a = usb_host_data->a;
            report->b = usb_host_data->b;
            report->x = usb_host_data->x;
            report->leftBlue = usb_host_data->leftBlue;
            report->leftRed = usb_host_data->leftRed;
            report->leftGreen = usb_host_data->leftGreen;
            report->rightBlue = usb_host_data->rightBlue;
            report->rightRed = usb_host_data->rightRed;
            report->rightGreen = usb_host_data->rightGreen;
            report->euphoria = usb_host_data->euphoria;
            report->effectsKnob = (usb_host_data->effectsKnob + 0x0200) << 6;
            report->crossfader = (usb_host_data->crossfader + 0x0200) << 6;
            report->leftTableVelocity = (usb_host_data->leftTableVelocity >> 8) + 0x80;
            report->rightTableVelocity = (usb_host_data->rightTableVelocity >> 8) + 0x80;
            return sizeof(PS3Turntable_Data_t);
        }

        case ROCK_BAND_PRO_GUITAR_SQUIRE:
        case ROCK_BAND_PRO_GUITAR_MUSTANG: {
            PS3RockBandProGuitar_Data_t *report = (PS3RockBandProGuitar_Data_t *)data;
            report->lowEFretVelocity = usb_host_data->lowEFretVelocity;
            report->aFretVelocity = usb_host_data->aFretVelocity;
            report->dFretVelocity = usb_host_data->dFretVelocity;
            report->gFretVelocity = usb_host_data->gFretVelocity;
            report->bFretVelocity = usb_host_data->bFretVelocity;
            report->highEFretVelocity = usb_host_data->highEFretVelocity;
            report->a = usb_host_data->a;
            report->b = usb_host_data->b;
            report->x = usb_host_data->x;
            report->y = usb_host_data->y;
            report->green = usb_host_data->green;
            report->red = usb_host_data->red;
            report->yellow = usb_host_data->yellow;
            report->blue = usb_host_data->blue;
            report->orange = usb_host_data->orange;
            report->lowEFret = usb_host_data->lowEFret;
            report->aFret = usb_host_data->aFret;
            report->dFret = usb_host_data->dFret;
            report->gFret = usb_host_data->gFret;
            report->bFret = usb_host_data->bFret;
            report->highEFret = usb_host_data->highEFret;
            report->tilt = usb_host_data->tilt;
            report->kick1 = usb_host_data->kick1;
            return sizeof(PS3RockBandProGuitar_Data_t);
        }
    }
    return 0;
}