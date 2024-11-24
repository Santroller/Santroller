#include <stdint.h>

#include "defines.h"
#include "config.h"
#include "reports/controller_reports.h"
#include "state_translation/x360.h"
#include "state_translation/ogxbox.h"
void ogxbox_to_universal_report(const uint8_t *data, uint8_t len, uint8_t sub_type, USB_Host_Data_t *usb_host_data) {
    OGXboxGamepad_Data_t *report = (OGXboxGamepad_Data_t *)data;
    usb_host_data->dpadLeft |= report->dpadLeft;
    usb_host_data->dpadRight |= report->dpadRight;
    usb_host_data->dpadUp |= report->dpadUp;
    usb_host_data->dpadDown |= report->dpadDown;
    usb_host_data->start |= report->start;
    usb_host_data->back |= report->back;
    switch (sub_type) {
        case XINPUT_GUITAR_BASS:
        case XINPUT_GUITAR: {
            OGXboxRockBandGuitar_Data_t *report = (OGXboxRockBandGuitar_Data_t *)data;
            usb_host_data->green |= report->green;
            usb_host_data->red |= report->red;
            usb_host_data->yellow |= report->yellow;
            usb_host_data->blue |= report->blue;
            usb_host_data->orange |= report->orange;
            if (report->tilt) {
                usb_host_data->tilt = (report->tilt >> 8) + 0x80;
            }
            if (report->whammy) {
                usb_host_data->whammy = (report->whammy >> 8) + 0x80;
            }
            if (report->leftStickX) {
                usb_host_data->leftStickX = (report->leftStickX >> 8) + 0x80;
            }
            if (report->leftStickY) {
                usb_host_data->leftStickY = (-report->leftStickY >> 8) + 0x80;
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
        case XINPUT_GUITAR_WT:
        case XINPUT_GUITAR_ALTERNATE: {
            OGXboxGuitarHeroGuitar_Data_t *report = (OGXboxGuitarHeroGuitar_Data_t *)data;
            uint8_t slider = (report->slider >> 8) ^ 0x80;
            usb_host_data->green |= report->green;
            usb_host_data->red |= report->red;
            usb_host_data->yellow |= report->yellow;
            usb_host_data->blue |= report->blue;
            usb_host_data->orange |= report->orange;
            usb_host_data->kick1 |= report->kick1;
            if (report->tilt) {
                usb_host_data->tilt = report->tilt;
            }
            if (report->whammy) {
                usb_host_data->whammy = (report->whammy + 0x7fff) >> 8;
            }
            if (report->slider) {
                usb_host_data->slider = usb_host_data->slider;
            };
            if (sub_type == 26) {
                if (slider < 0x2F) {
                    usb_host_data->slider = 0x15;
                } else if (slider <= 0x3F) {
                    usb_host_data->slider = 0x30;
                } else if (slider <= 0x5F) {
                    usb_host_data->slider = 0x4D;
                } else if (slider <= 0x6F) {
                    usb_host_data->slider = 0x66;
                } else if (slider <= 0x8F) {
                    usb_host_data->slider = 0x80;
                } else if (slider <= 0x9F) {
                    usb_host_data->slider = 0x9A;
                } else if (slider <= 0xAF) {
                    usb_host_data->slider = 0xAF;
                } else if (slider <= 0xCF) {
                    usb_host_data->slider = 0xC9;
                } else if (slider <= 0xEF) {
                    usb_host_data->slider = 0xE6;
                } else {
                    usb_host_data->slider = 0xFF;
                }
            };
            break;
        }
        case XINPUT_DRUMS: {
            OGXboxGamepad_Data_t *gamepad = (OGXboxGamepad_Data_t *)data;
            // leftThumbClick is true for guitar hero, false for rockband
            if (gamepad->leftThumbClick) {
                OGXboxGuitarHeroDrums_Data_t *report = (OGXboxGuitarHeroDrums_Data_t *)data;
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
            } else {
                OGXboxRockBandDrums_Data_t *report = (OGXboxRockBandDrums_Data_t *)data;
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
                        usb_host_data->greenCymbalVelocity = -((0x7FFF - report->greenVelocity) >> 7);
                    } else {
                        usb_host_data->greenVelocity = -((0x7FFF - report->greenVelocity) >> 7);
                    }
                }
                if (report->redVelocity) {
                    usb_host_data->redVelocity = (0x7FFF - report->redVelocity) >> 7;
                }
                if (report->yellowVelocity) {
                    if (usb_host_data->yellowCymbal) {
                        usb_host_data->yellowCymbalVelocity = -((0x7FFF - report->yellowVelocity) >> 7);
                    } else {
                        usb_host_data->yellowVelocity = -((0x7FFF - report->yellowVelocity) >> 7);
                    }
                }
                if (report->blueVelocity) {
                    if (usb_host_data->blueCymbal) {
                        usb_host_data->blueCymbalVelocity = (0x7FFF - report->blueVelocity) >> 7;
                    } else {
                        usb_host_data->blueVelocity = (0x7FFF - report->blueVelocity) >> 7;
                    }
                };
            }
            break;
        }
        case XINPUT_GUITAR_HERO_LIVE: {
            OGXboxGHLGuitar_Data_t *report = (OGXboxGHLGuitar_Data_t *)data;
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
                usb_host_data->tilt = report->tilt;
            }
            if (report->whammy) {
                usb_host_data->whammy = (report->whammy + 0x7fff) >> 8;
            }
            break;
        }
        case XINPUT_TURNTABLE: {
            OGXboxTurntable_Data_t *report = (OGXboxTurntable_Data_t *)data;
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
                usb_host_data->effectsKnob = report->effectsKnob;
            }
            if (report->crossfader) {
                usb_host_data->crossfader = report->crossfader;
            }
            if (report->leftTableVelocity) {
                usb_host_data->leftTableVelocity = report->leftTableVelocity;
            }
            if (report->rightTableVelocity) {
                usb_host_data->rightTableVelocity = report->rightTableVelocity;
            };
            break;
        }
        case XINPUT_PRO_KEYS: {
            OGXboxRockBandKeyboard_Data_t *report = (OGXboxRockBandKeyboard_Data_t *)data;
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
        case XINPUT_PRO_GUITAR: {
            OGXboxRockBandProGuitar_Data_t *report = (OGXboxRockBandProGuitar_Data_t *)data;
            usb_host_data->dpadLeft |= report->dpadLeft;
            usb_host_data->dpadRight |= report->dpadRight;
            usb_host_data->dpadUp |= report->dpadUp;
            usb_host_data->dpadDown |= report->dpadDown;
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
            ;
            break;
        }
        // Any other subtypes we dont handle can just be read like gamepads.
        default: {
            OGXboxGamepad_Data_t *report = (OGXboxGamepad_Data_t *)data;
            usb_host_data->a |= report->a > 0x20;
            usb_host_data->b |= report->b > 0x20;
            usb_host_data->x |= report->x > 0x20;
            usb_host_data->y |= report->y > 0x20;
            usb_host_data->leftShoulder |= report->leftShoulder > 0x20;
            usb_host_data->rightShoulder |= report->rightShoulder > 0x20;
            usb_host_data->back |= report->back;
            usb_host_data->start |= report->start;
            usb_host_data->leftThumbClick |= report->leftThumbClick;
            usb_host_data->rightThumbClick |= report->rightThumbClick;
            usb_host_data->dpadLeft = report->dpadLeft;
            usb_host_data->dpadRight = report->dpadRight;
            usb_host_data->dpadUp = report->dpadUp;
            usb_host_data->dpadDown = report->dpadDown;
            if (report->leftTrigger) {
                usb_host_data->leftTrigger = report->leftTrigger << 8;
            }
            if (report->rightTrigger) {
                usb_host_data->rightTrigger = report->rightTrigger << 8;
            }
            if (report->leftStickX) {
                usb_host_data->leftStickX = report->leftStickX;
            }
            if (report->leftStickY) {
                usb_host_data->leftStickY = report->leftStickY;
            }
            if (report->rightStickX) {
                usb_host_data->rightStickX = report->rightStickX;
            }
            if (report->rightStickY) {
                usb_host_data->rightStickY = report->rightStickY;
            }
            if (report->leftShoulder) {
                usb_host_data->pressureL1 = report->leftShoulder;
            }
            if (report->rightShoulder) {
                usb_host_data->rightShoulder = report->rightShoulder;
            }
            if (report->y) {
                usb_host_data->pressureTriangle = report->y;
            }
            if (report->b) {
                usb_host_data->pressureCircle = report->b;
            }
            if (report->a) {
                usb_host_data->pressureCross = report->a;
            }
            if (report->x) {
                usb_host_data->pressureSquare = report->x;
            };
            break;
        }
    }
}

uint8_t universal_report_to_ogxbox(uint8_t *data, uint8_t sub_type, const USB_Host_Data_t *usb_host_data) {
    // Since none of the instruments on ogxbox exist, we just mirror the 360 reports
    XInputGamepad_Data_t dpad_report;
    universal_report_to_x360((uint8_t *)&dpad_report, sub_type, usb_host_data);
    OGXboxGamepad_Data_t *report = (OGXboxGamepad_Data_t *)data;
    report->a = dpad_report.a * 0xFF;
    report->b = dpad_report.b * 0xFF;
    report->x = dpad_report.x * 0xFF;
    report->y = dpad_report.y * 0xFF;
    report->leftShoulder = dpad_report.leftShoulder * 0xFF;
    report->rightShoulder = dpad_report.rightShoulder * 0xFF;
    report->leftTrigger = dpad_report.leftTrigger >> 8;
    report->rightTrigger = dpad_report.rightTrigger >> 8;
    report->leftStickX = dpad_report.leftStickX;
    report->leftStickY = dpad_report.leftStickY;
    report->rightStickX = dpad_report.rightStickX;
    report->rightStickY = dpad_report.rightStickY;
    report->leftThumbClick = dpad_report.leftThumbClick;
    report->rightThumbClick = dpad_report.rightThumbClick;
    return sizeof(OGXboxGamepad_Data_t);
}