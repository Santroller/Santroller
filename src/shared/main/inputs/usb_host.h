#ifdef INPUT_USB_HOST
uint8_t device_count = get_usb_host_device_count();
bool poke_ghl = false;

if (millis() - lastSentGHLPoke > 8000) {
    poke_ghl = true;
    lastSentGHLPoke = millis();
}
memset(&usb_host_data, 0, sizeof(usb_host_data));
for (int i = 0; i < device_count; i++) {
    USB_Device_Type_t device_type = get_usb_host_device_type(i);
    // Poke any GHL guitars to keep em alive
    if (poke_ghl && device_type.sub_type == LIVE_GUITAR) {
        if (device_type.console_type == PS3) {
            transfer_with_usb_controller(device_type.dev_addr, USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS, HID_REQUEST_SET_REPORT, 0x0201, 0, sizeof(ghl_ps3wiiu_magic_data), ghl_ps3wiiu_magic_data);
        } else if (device_type.console_type == PS4) {
            transfer_with_usb_controller(device_type.dev_addr, USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS, HID_REQUEST_SET_REPORT, 0x0201, 0, sizeof(ghl_ps4_magic_data), ghl_ps4_magic_data);
        } else if (device_type.console_type == XBOXONE) {
            if (ghl_sequence_number_host == 0) {
                ghl_sequence_number_host = 1;
            }
            XboxOneGHLGuitar_Output_t data;
            XboxOneGHLGuitar_Output_t *report = &data;
            GIP_HEADER(report, GHL_HID_OUTPUT, false, ghl_sequence_number_host++);
            data.sub_command = 0x02;
            data.data[0] = 0x08;
            data.data[1] = 0x0A;
            send_report_to_controller(device_type.dev_addr, (uint8_t *)&data, sizeof(data));
        }
    }
    uint8_t *data = (uint8_t *)&temp_report_usb_host;
    get_usb_host_device_data(i, data);
    uint8_t console_type = device_type.console_type;
    if (console_type == XBOXONE) {
        GipHeader_t *header = (GipHeader_t *)data;
        if (device_type.sub_type == LIVE_GUITAR && header->command == GHL_HID_REPORT) {
            // Xbox one GHL guitars actually end up using PS3 reports if you poke them.
            console_type = PS3;
            data = (uint8_t *)&((XboxOneGHLGuitar_Data_t *)data)->report;
        } else if (header->command != GIP_INPUT_REPORT) {
            // Not input data, continue
            continue;
        }
    }
    switch (console_type) {
        case UNIVERSAL: {
            USB_Host_Data_t *report = (USB_Host_Data_t *)data;
            usb_host_data.genericX = report->genericX;
            usb_host_data.genericY = report->genericY;
            usb_host_data.genericX = report->genericX;
            usb_host_data.genericRX = report->genericRX;
            usb_host_data.genericRY = report->genericRY;
            usb_host_data.genericRX = report->genericRX;
            usb_host_data.genericSlider = report->genericSlider;
            usb_host_data.dpadLeft |= report->dpadLeft;
            usb_host_data.dpadRight |= report->dpadRight;
            usb_host_data.dpadUp |= report->dpadUp;
            usb_host_data.dpadDown |= report->dpadDown;
            usb_host_data.genericButtons |= report->genericButtons;
            break;
        }
        case PS3: {
            PS3Dpad_Data_t *report = (PS3Dpad_Data_t *)data;
            uint8_t dpad = report->dpad >= 0x08 ? 0 : dpad_bindings_reverse[report->dpad];
            bool up = dpad & UP;
            bool left = dpad & LEFT;
            bool down = dpad & DOWN;
            bool right = dpad & RIGHT;
            switch (device_type.sub_type) {
                case GAMEPAD: {
                    PS3Gamepad_Data_t *report = (PS3Gamepad_Data_t *)data;
                    usb_host_data.green |= report->a;
                    usb_host_data.red |= report->b;
                    usb_host_data.yellow |= report->y;
                    usb_host_data.blue |= report->x;
                    usb_host_data.orange |= report->leftShoulder;
                    usb_host_data.a |= report->a;
                    usb_host_data.b |= report->b;
                    usb_host_data.x |= report->x;
                    usb_host_data.y |= report->y;
                    usb_host_data.leftShoulder |= report->leftShoulder;
                    usb_host_data.rightShoulder |= report->rightShoulder;
                    usb_host_data.back |= report->back;
                    usb_host_data.start |= report->start;
                    usb_host_data.guide |= report->guide;
                    usb_host_data.leftThumbClick |= report->leftThumbClick;
                    usb_host_data.rightThumbClick |= report->rightThumbClick;
                    usb_host_data.dpadLeft |= report->dpadLeft;
                    usb_host_data.dpadRight |= report->dpadRight;
                    usb_host_data.dpadUp |= report->dpadUp;
                    usb_host_data.dpadDown |= report->dpadDown;
                    if (report->leftTrigger) {
                        usb_host_data.leftTrigger = report->leftTrigger << 8;
                    }
                    if (report->rightTrigger) {
                        usb_host_data.rightTrigger = report->rightTrigger << 8;
                    }
                    if (report->leftStickX != PS3_STICK_CENTER) {
                        usb_host_data.leftStickX = (report->leftStickX - PS3_STICK_CENTER) << 8;
                    }
                    if (report->leftStickY != PS3_STICK_CENTER) {
                        usb_host_data.leftStickY = (((UINT8_MAX - report->leftStickY) - PS3_STICK_CENTER)) << 8;
                    }
                    if (report->rightStickX != PS3_STICK_CENTER) {
                        usb_host_data.rightStickX = (report->rightStickX - PS3_STICK_CENTER) << 8;
                    }
                    if (report->rightStickY != PS3_STICK_CENTER) {
                        usb_host_data.rightStickY = (((UINT8_MAX - report->rightStickY) - PS3_STICK_CENTER)) << 8;
                    }
                    if (report->pressureDpadUp) {
                        usb_host_data.pressureDpadUp = report->pressureDpadUp;
                    }
                    if (report->pressureDpadRight) {
                        usb_host_data.pressureDpadRight = report->pressureDpadRight;
                    }
                    if (report->pressureDpadDown) {
                        usb_host_data.pressureDpadDown = report->pressureDpadDown;
                    }
                    if (report->pressureDpadLeft) {
                        usb_host_data.pressureDpadLeft = report->pressureDpadLeft;
                    }
                    if (report->pressureL1) {
                        usb_host_data.pressureL1 = report->pressureL1;
                    }
                    if (report->pressureR1) {
                        usb_host_data.pressureR1 = report->pressureR1;
                    }
                    if (report->pressureTriangle) {
                        usb_host_data.pressureTriangle = report->pressureTriangle;
                    }
                    if (report->pressureCircle) {
                        usb_host_data.pressureCircle = report->pressureCircle;
                    }
                    if (report->pressureCross) {
                        usb_host_data.pressureCross = report->pressureCross;
                    }
                    if (report->pressureSquare) {
                        usb_host_data.pressureSquare = report->pressureSquare;
                    }
                    break;
                }
                case ROCK_BAND_GUITAR: {
                    PS3RockBandGuitar_Data_t *report = (PS3RockBandGuitar_Data_t *)data;
                    usb_host_data.a |= report->a;
                    usb_host_data.b |= report->b;
                    usb_host_data.x |= report->x;
                    usb_host_data.y |= report->y;
                    usb_host_data.leftShoulder |= report->leftShoulder;
                    usb_host_data.green |= report->a;
                    usb_host_data.red |= report->b;
                    usb_host_data.yellow |= report->y;
                    usb_host_data.blue |= report->x;
                    usb_host_data.orange |= report->leftShoulder;
                    usb_host_data.back |= report->back;
                    usb_host_data.start |= report->start;
                    usb_host_data.guide |= report->guide;
                    usb_host_data.dpadLeft |= left;
                    usb_host_data.dpadRight |= right;
                    usb_host_data.dpadUp |= up;
                    usb_host_data.dpadDown |= down;
                    if (report->tilt) {
                        usb_host_data.tilt = INT16_MAX;
                    }
                    if (report->solo) {
                        usb_host_data.soloGreen |= report->a;
                        usb_host_data.soloRed |= report->b;
                        usb_host_data.soloYellow |= report->y;
                        usb_host_data.soloBlue |= report->x;
                        usb_host_data.soloOrange |= report->leftShoulder;
                    }
                    if (report->whammy) {
                        usb_host_data.whammy = report->whammy;
                    }
                    if (report->pickup) {
                        usb_host_data.pickup = report->pickup;
                    }
                    break;
                }
                case GUITAR_HERO_GUITAR_WT: 
                case GUITAR_HERO_GUITAR: {
                    PS3GuitarHeroGuitar_Data_t *report = (PS3GuitarHeroGuitar_Data_t *)data;
                    usb_host_data.a |= report->a;
                    usb_host_data.b |= report->b;
                    usb_host_data.x |= report->x;
                    usb_host_data.y |= report->y;
                    usb_host_data.leftShoulder |= report->leftShoulder;
                    usb_host_data.green |= report->a;
                    usb_host_data.red |= report->b;
                    usb_host_data.yellow |= report->y;
                    usb_host_data.blue |= report->x;
                    usb_host_data.orange |= report->leftShoulder;
                    usb_host_data.back |= report->back;
                    usb_host_data.start |= report->start;
                    usb_host_data.guide |= report->guide;
                    usb_host_data.dpadLeft |= left;
                    usb_host_data.dpadRight |= right;
                    usb_host_data.dpadUp |= up;
                    usb_host_data.dpadDown |= down;
                    if (report->tilt != PS3_ACCEL_CENTER) {
                        usb_host_data.tilt = (report->tilt - PS3_ACCEL_CENTER) << 6;
                    }
                    if (report->whammy) {
                        usb_host_data.whammy = report->whammy;
                    }
                    // Detect GH5 vs WT. Wait for a neutral input, then use that to detect instrument type
                    if (device_type.sub_type == GUITAR_HERO_GUITAR_WT) {
                        // Its WT, convert to GH5
                        if (report->slider <= 0x2F) {
                            usb_host_data.slider = 0x95;
                        } else if (report->slider <= 0x3F) {
                            usb_host_data.slider = 0xB0;
                        } else if (report->slider <= 0x5F) {
                            usb_host_data.slider = 0xCD;
                        } else if (report->slider <= 0x6F) {
                            usb_host_data.slider = 0xE6;
                        } else if (report->slider <= 0x8F) {
                            usb_host_data.slider = 0;
                        } else if (report->slider <= 0x9F) {
                            usb_host_data.slider = 0x1A;
                        } else if (report->slider <= 0xAF) {
                            usb_host_data.slider = 0x2F;
                        } else if (report->slider <= 0xCF) {
                            usb_host_data.slider = 0x49;
                        } else if (report->slider <= 0xEF) {
                            usb_host_data.slider = 0x66;
                        } else {
                            usb_host_data.slider = 0x7F;
                        }
                    } else {
                        usb_host_data.slider = report->slider + 0x80;
                    }
                    break;
                }
                case ROCK_BAND_DRUMS: {
                    PS3RockBandDrums_Data_t *report = (PS3RockBandDrums_Data_t *)data;
                    usb_host_data.a |= report->a;
                    usb_host_data.b |= report->b;
                    usb_host_data.x |= report->x;
                    usb_host_data.y |= report->y;
                    usb_host_data.kick1 |= report->leftShoulder;
                    usb_host_data.kick2 |= report->rightShoulder;
                    usb_host_data.back |= report->back;
                    usb_host_data.start |= report->start;
                    usb_host_data.guide |= report->guide;
                    usb_host_data.green |= report->a && report->padFlag;
                    usb_host_data.red |= report->b && report->padFlag;
                    usb_host_data.yellow |= report->y && report->padFlag;
                    usb_host_data.blue |= report->x && report->padFlag;
                    usb_host_data.greenCymbal |= report->a && report->cymbalFlag;
                    usb_host_data.blueCymbal |= report->x && report->cymbalFlag && up;
                    usb_host_data.yellowCymbal |= report->y && report->cymbalFlag && down;
                    usb_host_data.dpadLeft |= left;
                    usb_host_data.dpadRight |= right;
                    usb_host_data.dpadUp |= up;
                    usb_host_data.dpadDown |= down;
                    if (usb_host_data.kick1 || usb_host_data.kick2) {
                        usb_host_data.kickVelocity = 0xFF;
                    }
                    if (report->greenVelocity) {
                        if (usb_host_data.greenCymbal) {
                            usb_host_data.greenCymbalVelocity = report->greenVelocity;
                        } else {
                            usb_host_data.greenVelocity = report->greenVelocity;
                        }
                    }
                    if (report->redVelocity) {
                        usb_host_data.redVelocity = report->redVelocity;
                    }
                    if (report->yellowVelocity) {
                        if (usb_host_data.yellowCymbal) {
                            usb_host_data.yellowCymbalVelocity = report->yellowVelocity;
                        } else {
                            usb_host_data.yellowVelocity = report->yellowVelocity;
                        }
                    }
                    if (report->blueVelocity) {
                        if (usb_host_data.blueCymbal) {
                            usb_host_data.blueCymbalVelocity = report->blueVelocity;
                        } else {
                            usb_host_data.blueVelocity = report->blueVelocity;
                        }
                    }
                    break;
                }
                case GUITAR_HERO_DRUMS: {
                    PS3GuitarHeroDrums_Data_t *report = (PS3GuitarHeroDrums_Data_t *)data;
                    usb_host_data.a |= report->a;
                    usb_host_data.b |= report->b;
                    usb_host_data.x |= report->x;
                    usb_host_data.y |= report->y;
                    usb_host_data.leftShoulder |= report->leftShoulder;
                    usb_host_data.back |= report->back;
                    usb_host_data.start |= report->start;
                    usb_host_data.guide |= report->guide;
                    usb_host_data.dpadLeft |= left;
                    usb_host_data.dpadRight |= right;
                    usb_host_data.dpadUp |= up;
                    usb_host_data.dpadDown |= down;
                    if (report->greenVelocity) {
                        usb_host_data.greenVelocity = report->greenVelocity;
                    }
                    if (report->redVelocity) {
                        usb_host_data.redVelocity = report->redVelocity;
                    }
                    if (report->yellowVelocity) {
                        usb_host_data.yellowVelocity = report->yellowVelocity;
                    }
                    if (report->blueVelocity) {
                        usb_host_data.blueVelocity = report->blueVelocity;
                    }
                    if (report->orangeVelocity) {
                        usb_host_data.orangeVelocity = report->orangeVelocity;
                    }
                    if (report->kickVelocity) {
                        usb_host_data.kickVelocity = report->kickVelocity;
                        usb_host_data.kick1 = true;
                    }
                    break;
                }
                case LIVE_GUITAR: {
                    PS3GHLGuitar_Data_t *report = (PS3GHLGuitar_Data_t *)data;
                    usb_host_data.a |= report->a;
                    usb_host_data.b |= report->b;
                    usb_host_data.x |= report->x;
                    usb_host_data.y |= report->y;
                    usb_host_data.leftShoulder |= report->leftShoulder;
                    usb_host_data.rightShoulder |= report->rightShoulder;
                    usb_host_data.dpadLeft |= left;
                    usb_host_data.dpadRight |= right;
                    usb_host_data.dpadUp |= up;
                    usb_host_data.dpadDown |= down;
                    usb_host_data.leftThumbClick |= report->leftThumbClick;
                    usb_host_data.back |= report->back;
                    usb_host_data.start |= report->start;
                    usb_host_data.guide |= report->guide;
                    if (report->tilt != PS3_ACCEL_CENTER) {
                        usb_host_data.tilt = (report->tilt - PS3_ACCEL_CENTER) << 6;
                    }
                    if (report->whammy) {
                        usb_host_data.whammy = report->whammy << 8;
                    }
                    break;
                }
                case DJ_HERO_TURNTABLE: {
                    PS3Turntable_Data_t *report = (PS3Turntable_Data_t *)data;
                    usb_host_data.a |= report->a;
                    usb_host_data.b |= report->b;
                    usb_host_data.x |= report->x;
                    usb_host_data.y |= report->y;
                    usb_host_data.dpadLeft |= left;
                    usb_host_data.dpadRight |= right;
                    usb_host_data.dpadUp |= up;
                    usb_host_data.dpadDown |= down;
                    usb_host_data.back |= report->back;
                    usb_host_data.start |= report->start;
                    usb_host_data.guide |= report->guide;
                    usb_host_data.leftBlue |= report->leftBlue;
                    usb_host_data.leftRed |= report->leftRed;
                    usb_host_data.leftGreen |= report->leftGreen;
                    usb_host_data.rightBlue |= report->rightBlue;
                    usb_host_data.rightRed |= report->rightRed;
                    usb_host_data.rightGreen |= report->rightGreen;
                    if (report->effectsKnob != PS3_ACCEL_CENTER) {
                        usb_host_data.effectsKnob = (report->effectsKnob - PS3_ACCEL_CENTER) << 6;
                    }
                    if (report->crossfader != PS3_ACCEL_CENTER) {
                        usb_host_data.crossfader = (report->crossfader - PS3_ACCEL_CENTER) << 6;
                    }
                    if (report->leftTableVelocity != PS3_STICK_CENTER) {
                        usb_host_data.leftTableVelocity = (report->leftTableVelocity - PS3_STICK_CENTER) << 8;
                    }
                    if (report->rightTableVelocity != PS3_STICK_CENTER) {
                        usb_host_data.rightTableVelocity = (report->rightTableVelocity - PS3_STICK_CENTER) << 8;
                    }
                    break;
                }
            }
            break;
        }
        case PS4: {
            PS4Dpad_Data_t *dpad = (PS4Dpad_Data_t *)data;
            usb_host_data.dpadLeft = dpad->dpad == 6 || dpad->dpad == 5 || dpad->dpad == 7;
            usb_host_data.dpadRight = dpad->dpad == 3 || dpad->dpad == 2 || dpad->dpad == 1;
            usb_host_data.dpadUp = dpad->dpad == 0 || dpad->dpad == 1 || dpad->dpad == 7;
            usb_host_data.dpadDown = dpad->dpad == 5 || dpad->dpad == 4 || dpad->dpad == 3;
            switch (device_type.sub_type) {
                case GAMEPAD: {
                    PS4Gamepad_Data_t *report = (PS4Gamepad_Data_t *)data;
                    usb_host_data.green |= report->a;
                    usb_host_data.red |= report->b;
                    usb_host_data.yellow |= report->y;
                    usb_host_data.blue |= report->x;
                    usb_host_data.orange |= report->leftShoulder;
                    usb_host_data.a |= report->a;
                    usb_host_data.b |= report->b;
                    usb_host_data.x |= report->x;
                    usb_host_data.y |= report->y;
                    usb_host_data.leftShoulder |= report->leftShoulder;
                    usb_host_data.rightShoulder |= report->rightShoulder;
                    usb_host_data.back |= report->back;
                    usb_host_data.start |= report->start;
                    usb_host_data.guide |= report->guide;
                    usb_host_data.leftThumbClick |= report->leftThumbClick;
                    usb_host_data.rightThumbClick |= report->rightThumbClick;
                    if (report->leftTrigger) {
                        usb_host_data.leftTrigger = report->leftTrigger << 8;
                    }
                    if (report->rightTrigger) {
                        usb_host_data.rightTrigger = report->rightTrigger << 8;
                    }
                    if (report->leftStickX != PS3_STICK_CENTER) {
                        usb_host_data.leftStickX = (report->leftStickX - PS3_STICK_CENTER) << 8;
                    }
                    if (report->leftStickY != PS3_STICK_CENTER) {
                        usb_host_data.leftStickY = (((UINT8_MAX - report->leftStickY) - PS3_STICK_CENTER)) << 8;
                    }
                    if (report->rightStickX != PS3_STICK_CENTER) {
                        usb_host_data.rightStickX = (report->rightStickX - PS3_STICK_CENTER) << 8;
                    }
                    if (report->rightStickY != PS3_STICK_CENTER) {
                        usb_host_data.rightStickY = (((UINT8_MAX - report->rightStickY) - PS3_STICK_CENTER)) << 8;
                    }
                    break;
                }
                case LIVE_GUITAR: {
                    PS4GHLGuitar_Data_t *report = (PS4GHLGuitar_Data_t *)data;
                    usb_host_data.a |= report->a;
                    usb_host_data.b |= report->b;
                    usb_host_data.x |= report->x;
                    usb_host_data.y |= report->y;
                    usb_host_data.leftShoulder |= report->leftShoulder;
                    usb_host_data.rightShoulder |= report->rightShoulder;
                    usb_host_data.leftThumbClick |= report->leftThumbClick;
                    usb_host_data.back |= report->back;
                    usb_host_data.start |= report->start;
                    usb_host_data.guide |= report->guide;
                    if (report->tilt != PS3_ACCEL_CENTER) {
                        usb_host_data.tilt = (report->tilt - PS3_ACCEL_CENTER) << 6;
                    }
                    if (report->whammy) {
                        usb_host_data.whammy = report->whammy;
                    }
                    break;
                }
            }
            break;
        }
        case XBOX360: {
            switch (device_type.sub_type) {
                case XINPUT_GUITAR: {
                    XInputRockBandGuitar_Data_t *report = (XInputRockBandGuitar_Data_t *)data;
                    usb_host_data.a |= report->a;
                    usb_host_data.b |= report->b;
                    usb_host_data.x |= report->x;
                    usb_host_data.y |= report->y;
                    usb_host_data.leftShoulder |= report->leftShoulder;
                    usb_host_data.green |= report->a;
                    usb_host_data.red |= report->b;
                    usb_host_data.yellow |= report->y;
                    usb_host_data.blue |= report->x;
                    usb_host_data.orange |= report->leftShoulder;
                    usb_host_data.back |= report->back;
                    usb_host_data.start |= report->start;
                    usb_host_data.guide |= report->guide;
                    usb_host_data.dpadLeft = report->dpadLeft;
                    usb_host_data.dpadRight = report->dpadRight;
                    usb_host_data.dpadUp = report->dpadUp;
                    usb_host_data.dpadDown = report->dpadDown;
                    if (report->tilt) {
                        usb_host_data.tilt = INT16_MAX;
                    }
                    if (report->solo) {
                        usb_host_data.soloGreen |= report->a;
                        usb_host_data.soloRed |= report->b;
                        usb_host_data.soloYellow |= report->y;
                        usb_host_data.soloBlue |= report->x;
                        usb_host_data.soloOrange |= report->leftShoulder;
                    }
                    if (report->whammy) {
                        usb_host_data.whammy = report->whammy;
                    }
                    if (report->pickup) {
                        usb_host_data.pickup = report->pickup;
                    }
                    break;
                }
                case XINPUT_GUITAR_ALTERNATE: {
                    XInputGuitarHeroGuitar_Data_t *report = (XInputGuitarHeroGuitar_Data_t *)data;
                    usb_host_data.a |= report->a;
                    usb_host_data.b |= report->b;
                    usb_host_data.x |= report->x;
                    usb_host_data.y |= report->y;
                    usb_host_data.leftShoulder |= report->leftShoulder;
                    usb_host_data.green |= report->a;
                    usb_host_data.red |= report->b;
                    usb_host_data.yellow |= report->y;
                    usb_host_data.blue |= report->x;
                    usb_host_data.orange |= report->leftShoulder;
                    usb_host_data.back |= report->back;
                    usb_host_data.start |= report->start;
                    usb_host_data.guide |= report->guide;
                    usb_host_data.dpadLeft = report->dpadLeft;
                    usb_host_data.dpadRight = report->dpadRight;
                    usb_host_data.dpadUp = report->dpadUp;
                    usb_host_data.dpadDown = report->dpadDown;
                    if (report->tilt) {
                        usb_host_data.tilt = report->tilt;
                    }
                    if (report->whammy) {
                        usb_host_data.whammy = report->whammy;
                    }

                    // uint8_t slider = (report->slider >> 8) ^ 0x80;

                    // TODO: eventually if we implement wireless receiver support then this might be worth it
                    // We would then just check the vid and pid and do it that way
                    // if (slider < 0x2F) {
                    //     usb_host_data.slider = 0x95;
                    // } else if (slider <= 0x3F) {
                    //     usb_host_data.slider = 0xB0;
                    // } else if (slider <= 0x5F) {
                    //     usb_host_data.slider = 0xCD;
                    // } else if (slider <= 0x6F) {
                    //     usb_host_data.slider = 0xE6;
                    // } else if (slider <= 0x8F) {
                    //     usb_host_data.slider = 0;
                    // } else if (slider <= 0x9F) {
                    //     usb_host_data.slider = 0x1A;
                    // } else if (slider <= 0xAF) {
                    //     usb_host_data.slider = 0x2F;
                    // } else if (slider <= 0xCF) {
                    //     usb_host_data.slider = 0x49;
                    // } else if (slider <= 0xEF) {
                    //     usb_host_data.slider = 0x66;
                    // } else {
                    //     usb_host_data.slider = 0x7F;
                    // }
                    break;
                }
                case XINPUT_DRUMS: {
                    XInputGamepad_Data_t *gamepad = (XInputGamepad_Data_t *)data;
                    // leftThumbClick is true for guitar hero, false for rockband
                    if (gamepad->leftThumbClick) {
                        XInputGuitarHeroDrums_Data_t *report = (XInputGuitarHeroDrums_Data_t *)data;
                        usb_host_data.a |= report->a;
                        usb_host_data.b |= report->b;
                        usb_host_data.x |= report->x;
                        usb_host_data.y |= report->y;
                        usb_host_data.leftShoulder |= report->leftShoulder;
                        usb_host_data.back |= report->back;
                        usb_host_data.start |= report->start;
                        usb_host_data.guide |= report->guide;
                        usb_host_data.dpadLeft = report->dpadLeft;
                        usb_host_data.dpadRight = report->dpadRight;
                        usb_host_data.dpadUp = report->dpadUp;
                        usb_host_data.dpadDown = report->dpadDown;
                        if (report->greenVelocity) {
                            usb_host_data.greenVelocity = report->greenVelocity;
                        }
                        if (report->redVelocity) {
                            usb_host_data.redVelocity = report->redVelocity;
                        }
                        if (report->yellowVelocity) {
                            usb_host_data.yellowVelocity = report->yellowVelocity;
                        }
                        if (report->blueVelocity) {
                            usb_host_data.blueVelocity = report->blueVelocity;
                        }
                        if (report->orangeVelocity) {
                            usb_host_data.orangeVelocity = report->orangeVelocity;
                        }
                        if (report->kickVelocity) {
                            usb_host_data.kickVelocity = report->kickVelocity;
                            usb_host_data.kick1 = true;
                        }
                    } else {
                        XInputRockBandDrums_Data_t *report = (XInputRockBandDrums_Data_t *)data;
                        usb_host_data.a |= report->a;
                        usb_host_data.b |= report->b;
                        usb_host_data.x |= report->x;
                        usb_host_data.y |= report->y;
                        usb_host_data.dpadLeft = report->dpadLeft;
                        usb_host_data.dpadRight = report->dpadRight;
                        usb_host_data.dpadUp = report->dpadUp;
                        usb_host_data.dpadDown = report->dpadDown;
                        usb_host_data.kick1 |= report->leftShoulder;
                        usb_host_data.kick2 |= report->leftThumbClick;
                        usb_host_data.back |= report->back;
                        usb_host_data.start |= report->start;
                        usb_host_data.guide |= report->guide;
                        usb_host_data.green |= report->a && report->padFlag;
                        usb_host_data.red |= report->b && report->padFlag;
                        usb_host_data.yellow |= report->y && report->padFlag;
                        usb_host_data.blue |= report->x && report->padFlag;
                        usb_host_data.greenCymbal |= report->a && report->cymbalFlag;
                        usb_host_data.blueCymbal |= report->x && report->cymbalFlag && report->dpadUp;
                        usb_host_data.yellowCymbal |= report->y && report->cymbalFlag && report->dpadDown;
                        if (usb_host_data.kick1 || usb_host_data.kick2) {
                            usb_host_data.kickVelocity = 0xFF;
                        }
                        if (report->greenVelocity) {
                            if (usb_host_data.greenCymbal) {
                                usb_host_data.greenCymbalVelocity = (0x7FFF - report->greenVelocity) >> 7;
                            } else {
                                usb_host_data.greenVelocity = (0x7FFF - report->greenVelocity) >> 7;
                            }
                        }
                        if (report->redVelocity) {
                            usb_host_data.redVelocity = (0x7FFF - report->redVelocity) >> 7;
                        }
                        if (report->yellowVelocity) {
                            if (usb_host_data.yellowCymbal) {
                                usb_host_data.yellowCymbalVelocity = (0x7FFF - report->yellowVelocity) >> 7;
                            } else {
                                usb_host_data.yellowVelocity = (0x7FFF - report->yellowVelocity) >> 7;
                            }
                        }
                        if (report->blueVelocity) {
                            if (usb_host_data.blueCymbal) {
                                usb_host_data.blueCymbalVelocity = (0x7FFF - report->blueVelocity) >> 7;
                            } else {
                                usb_host_data.blueVelocity = (0x7FFF - report->blueVelocity) >> 7;
                            }
                        }
                    }
                    break;
                }
                case XINPUT_GUITAR_HERO_LIVE: {
                    XInputGHLGuitar_Data_t *report = (XInputGHLGuitar_Data_t *)data;
                    usb_host_data.a |= report->a;
                    usb_host_data.b |= report->b;
                    usb_host_data.x |= report->x;
                    usb_host_data.y |= report->y;
                    usb_host_data.leftShoulder |= report->leftShoulder;
                    usb_host_data.rightShoulder |= report->rightShoulder;
                    usb_host_data.dpadLeft = report->dpadLeft;
                    usb_host_data.dpadRight = report->dpadRight;
                    usb_host_data.dpadUp = report->dpadUp;
                    usb_host_data.dpadDown = report->dpadDown;
                    usb_host_data.leftThumbClick |= report->leftThumbClick;
                    usb_host_data.back |= report->back;
                    usb_host_data.start |= report->start;
                    usb_host_data.guide |= report->guide;
                    if (report->tilt) {
                        usb_host_data.tilt = report->tilt;
                    }
                    if (report->whammy) {
                        usb_host_data.whammy = report->whammy;
                    }
                    break;
                }
                case XINPUT_TURNTABLE: {
                    XInputTurntable_Data_t *report = (XInputTurntable_Data_t *)data;
                    usb_host_data.a |= report->a;
                    usb_host_data.b |= report->b;
                    usb_host_data.x |= report->x;
                    usb_host_data.y |= report->y;
                    usb_host_data.dpadLeft = report->dpadLeft;
                    usb_host_data.dpadRight = report->dpadRight;
                    usb_host_data.dpadUp = report->dpadUp;
                    usb_host_data.dpadDown = report->dpadDown;
                    usb_host_data.back |= report->back;
                    usb_host_data.start |= report->start;
                    usb_host_data.guide |= report->guide;
                    usb_host_data.leftBlue |= report->leftBlue;
                    usb_host_data.leftRed |= report->leftRed;
                    usb_host_data.leftGreen |= report->leftGreen;
                    usb_host_data.rightBlue |= report->rightBlue;
                    usb_host_data.rightRed |= report->rightRed;
                    usb_host_data.rightGreen |= report->rightGreen;
                    if (report->effectsKnob) {
                        usb_host_data.effectsKnob = report->effectsKnob;
                    }
                    if (report->crossfader) {
                        usb_host_data.crossfader = report->crossfader;
                    }
                    if (report->leftTableVelocity) {
                        usb_host_data.leftTableVelocity = report->leftTableVelocity;
                    }
                    if (report->rightTableVelocity) {
                        usb_host_data.rightTableVelocity = report->rightTableVelocity;
                    }
                    break;
                }
                // Any other subtypes we dont handle can just be read like gamepads.
                default: {
                    XInputGamepad_Data_t *report = (XInputGamepad_Data_t *)data;
                    usb_host_data.green |= report->a;
                    usb_host_data.red |= report->b;
                    usb_host_data.yellow |= report->y;
                    usb_host_data.blue |= report->x;
                    usb_host_data.orange |= report->leftShoulder;
                    usb_host_data.a |= report->a;
                    usb_host_data.b |= report->b;
                    usb_host_data.x |= report->x;
                    usb_host_data.y |= report->y;
                    usb_host_data.leftShoulder |= report->leftShoulder;
                    usb_host_data.rightShoulder |= report->rightShoulder;
                    usb_host_data.back |= report->back;
                    usb_host_data.start |= report->start;
                    usb_host_data.guide |= report->guide;
                    usb_host_data.leftThumbClick |= report->leftThumbClick;
                    usb_host_data.rightThumbClick |= report->rightThumbClick;
                    usb_host_data.dpadLeft = report->dpadLeft;
                    usb_host_data.dpadRight = report->dpadRight;
                    usb_host_data.dpadUp = report->dpadUp;
                    usb_host_data.dpadDown = report->dpadDown;
                    if (report->leftTrigger) {
                        usb_host_data.leftTrigger = report->leftTrigger << 8;
                    }
                    if (report->rightTrigger) {
                        usb_host_data.rightTrigger = report->rightTrigger << 8;
                    }
                    if (report->leftStickX) {
                        usb_host_data.leftStickX = report->leftStickX;
                    }
                    if (report->leftStickY) {
                        usb_host_data.leftStickY = report->leftStickY;
                    }
                    if (report->rightStickX) {
                        usb_host_data.rightStickX = report->rightStickX;
                    }
                    if (report->rightStickY) {
                        usb_host_data.rightStickY = report->rightStickY;
                    }
                    break;
                }
            }
            break;
        }
        case XBOXONE: {
            switch (device_type.sub_type) {
                case GAMEPAD: {
                    XboxOneGamepad_Data_t *report = (XboxOneGamepad_Data_t *)data;
                    usb_host_data.a |= report->a;
                    usb_host_data.b |= report->b;
                    usb_host_data.x |= report->x;
                    usb_host_data.y |= report->y;
                    usb_host_data.leftShoulder |= report->leftShoulder;
                    usb_host_data.green |= report->a;
                    usb_host_data.red |= report->b;
                    usb_host_data.yellow |= report->y;
                    usb_host_data.blue |= report->x;
                    usb_host_data.orange |= report->leftShoulder;
                    usb_host_data.rightShoulder |= report->rightShoulder;
                    usb_host_data.back |= report->back;
                    usb_host_data.start |= report->start;
                    usb_host_data.guide |= report->guide;
                    usb_host_data.leftThumbClick |= report->leftThumbClick;
                    usb_host_data.rightThumbClick |= report->rightThumbClick;
                    usb_host_data.dpadLeft = report->dpadLeft;
                    usb_host_data.dpadRight = report->dpadRight;
                    usb_host_data.dpadUp = report->dpadUp;
                    usb_host_data.dpadDown = report->dpadDown;
                    // XB1 reports range from 0 - 1024
                    if (report->leftTrigger) {
                        usb_host_data.leftTrigger = report->leftTrigger << 6;
                    }
                    if (report->rightTrigger) {
                        usb_host_data.rightTrigger = report->rightTrigger << 6;
                    }
                    if (report->leftStickX) {
                        usb_host_data.leftStickX = report->leftStickX;
                    }
                    if (report->leftStickY) {
                        usb_host_data.leftStickY = report->leftStickY;
                    }
                    if (report->rightStickX) {
                        usb_host_data.rightStickX = report->rightStickX;
                    }
                    if (report->rightStickY) {
                        usb_host_data.rightStickY = report->rightStickY;
                    }
                    break;
                }
                case ROCK_BAND_GUITAR: {
                    XboxOneRockBandGuitar_Data_t *report = (XboxOneRockBandGuitar_Data_t *)data;
                    usb_host_data.a |= report->a;
                    usb_host_data.b |= report->b;
                    usb_host_data.x |= report->x;
                    usb_host_data.y |= report->y;
                    usb_host_data.leftShoulder |= report->leftShoulder;
                    usb_host_data.back |= report->back;
                    usb_host_data.start |= report->start;
                    usb_host_data.guide |= report->guide;
                    usb_host_data.green |= report->green;
                    usb_host_data.red |= report->red;
                    usb_host_data.yellow |= report->yellow;
                    usb_host_data.blue |= report->blue;
                    usb_host_data.orange |= report->orange;
                    usb_host_data.soloGreen |= report->soloGreen;
                    usb_host_data.soloRed |= report->soloRed;
                    usb_host_data.soloYellow |= report->soloYellow;
                    usb_host_data.soloBlue |= report->soloBlue;
                    usb_host_data.soloOrange |= report->soloOrange;
                    if (report->tilt) {
                        usb_host_data.tilt = report->tilt << 8;
                    }
                    if (report->whammy) {
                        usb_host_data.whammy = report->whammy;
                    }
                    if (report->pickup) {
                        usb_host_data.pickup = report->pickup;
                    }
                    break;
                }
                case ROCK_BAND_DRUMS: {
                    XboxOneRockBandDrums_Data_t *report = (XboxOneRockBandDrums_Data_t *)data;
                    usb_host_data.a |= report->a;
                    usb_host_data.b |= report->b;
                    usb_host_data.x |= report->x;
                    usb_host_data.y |= report->y;
                    usb_host_data.kick1 |= report->leftShoulder;
                    usb_host_data.kick2 |= report->rightShoulder;
                    usb_host_data.back |= report->back;
                    usb_host_data.start |= report->start;
                    usb_host_data.guide |= report->guide;
                    if (report->greenVelocity) {
                        usb_host_data.green = true;
                        usb_host_data.greenVelocity = report->greenVelocity << 4;
                    }
                    if (report->redVelocity) {
                        usb_host_data.red = true;
                        usb_host_data.redVelocity = report->redVelocity << 4;
                    }
                    if (report->yellowVelocity) {
                        usb_host_data.yellow = true;
                        usb_host_data.yellowVelocity = report->yellowVelocity << 4;
                    }
                    if (report->blueVelocity) {
                        usb_host_data.blue = true;
                        usb_host_data.blueVelocity = report->blueVelocity << 4;
                    }
                    if (report->blueCymbalVelocity) {
                        usb_host_data.blueCymbal = true;
                        usb_host_data.blueCymbalVelocity = report->blueCymbalVelocity << 4;
                    }
                    if (report->yellowCymbalVelocity) {
                        usb_host_data.yellowCymbal = true;
                        usb_host_data.yellowCymbalVelocity = report->yellowCymbalVelocity << 4;
                    }
                    if (report->greenCymbalVelocity) {
                        usb_host_data.greenCymbal = true;
                        usb_host_data.greenCymbalVelocity = report->greenCymbalVelocity << 4;
                    }
                    if (report->leftShoulder || report->rightShoulder) {
                        usb_host_data.kickVelocity = 0xFF;
                    }
                    break;
                }
            }
            break;
        }
    }
}
memcpy(&last_usb_host_data, &usb_host_data, sizeof(last_usb_host_data));
#endif