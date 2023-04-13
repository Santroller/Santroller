// Manually map every instrument
// By defining a bunch of macros, we can reuse this for each console output type
#define COPY_NORMAL_PS3(in, out) COPY_AXIS_NORMAL(((in + 128) << 8), out)
#define COPY_TRIGGER_PS3_XINPUT(in, out) COPY_AXIS_TRIGGER(in << 8, out)
#ifdef INPUT_USB_HOST

for (int i = 0; i < device_count; i++) {
    USB_Device_Type_t device_type = get_usb_host_device_type(i);
    get_usb_host_device_data(i, (uint8_t *)&temp_report_usb_host);
    void *data = &temp_report_usb_host;
    uint8_t console_type = device_type.console_type;
    if (console_type == XBOXONE) {
        GipHeader_t *header = (GipHeader_t *)data;
        if (device_type.sub_type == LIVE_GUITAR && header->command == GHL_HID_REPORT) {
            // Xbox one GHL guitars actually end up using PS3 reports if you poke them.
            console_type = PS3;
            data = &((XboxOneGHLGuitar_Data_t *)data)->report;
        } else if (header->command != GIP_INPUT_REPORT) {
            // Not input data, continue
            continue;
        }
    }
#if !DEVICE_TYPE_IS_INSTRUMENT
    switch (console_type) {
        case PS3: {
            if (device_type.sub_type == GAMEPAD) {
                PS3Gamepad_Data_t *host_gamepad = (PS3Gamepad_Data_t *)data;
                COPY_BUTTON(host_gamepad->back, report->back)
                COPY_BUTTON(host_gamepad->start, report->start)
                COPY_BUTTON(host_gamepad->leftThumbClick, report->leftThumbClick)
                COPY_BUTTON(host_gamepad->rightThumbClick, report->rightThumbClick)
                COPY_BUTTON(host_gamepad->dpadUp, report->dpadUp)
                COPY_BUTTON(host_gamepad->dpadDown, report->dpadDown)
                COPY_BUTTON(host_gamepad->dpadLeft, report->dpadLeft)
                COPY_BUTTON(host_gamepad->dpadRight, report->dpadRight)
                COPY_BUTTON(host_gamepad->leftShoulder, report->leftShoulder)
                COPY_BUTTON(host_gamepad->rightShoulder, report->rightShoulder)
#ifdef HAS_L2_R2_BUTTON
                COPY_BUTTON(host_gamepad->l2, report->l2)
                COPY_BUTTON(host_gamepad->r2, report->r2)
#endif
                COPY_BUTTON(host_gamepad->x, report->x)
                COPY_BUTTON(host_gamepad->a, report->a)
                COPY_BUTTON(host_gamepad->b, report->b)
                COPY_BUTTON(host_gamepad->y, report->y)
                COPY_BUTTON(host_gamepad->guide, report->guide)
                COPY_NORMAL_PS3(host_gamepad->leftStickX, report->leftStickX)
                COPY_NORMAL_PS3(host_gamepad->leftStickY, report->leftStickY)
                COPY_NORMAL_PS3(host_gamepad->rightStickX, report->rightStickX)
                COPY_NORMAL_PS3(host_gamepad->rightStickY, report->rightStickY)
                COPY_TRIGGER_PS3_XINPUT(host_gamepad->leftTrigger, report->leftTrigger)
                COPY_TRIGGER_PS3_XINPUT(host_gamepad->rightTrigger, report->rightTrigger)
            } else {
                PCGamepad_Data_t *host_gamepad = (PCGamepad_Data_t *)data;
                // Turn dpad back to bits so we can use them below
                PS3Dpad_Data_t *dpad = (PS3Dpad_Data_t *)dpad;
                dpad->dpad = dpad->dpad > RIGHT ? 0 : dpad_bindings_reverse[dpad->dpad];
                COPY_BUTTON(host_gamepad->back, report->back)
                COPY_BUTTON(host_gamepad->start, report->start)
                COPY_BUTTON(host_gamepad->leftThumbClick, report->leftThumbClick)
                COPY_BUTTON(host_gamepad->rightThumbClick, report->rightThumbClick)
                COPY_BUTTON(host_gamepad->dpadUp, report->dpadUp)
                COPY_BUTTON(host_gamepad->dpadDown, report->dpadDown)
                COPY_BUTTON(host_gamepad->dpadLeft, report->dpadLeft)
                COPY_BUTTON(host_gamepad->dpadRight, report->dpadRight)
                COPY_BUTTON(host_gamepad->leftShoulder, report->leftShoulder)
                COPY_BUTTON(host_gamepad->rightShoulder, report->rightShoulder)

#ifdef HAS_L2_R2_BUTTON
                COPY_BUTTON(host_gamepad->l2, report->l2)
                COPY_BUTTON(host_gamepad->r2, report->r2)
#endif
                COPY_BUTTON(host_gamepad->x, report->x)
                COPY_BUTTON(host_gamepad->a, report->a)
                COPY_BUTTON(host_gamepad->b, report->b)
                COPY_BUTTON(host_gamepad->y, report->y)
                COPY_BUTTON(host_gamepad->guide, report->guide)
                COPY_NORMAL_PS3(host_gamepad->leftStickX, report->leftStickX)
                COPY_NORMAL_PS3(host_gamepad->leftStickY, report->leftStickY)
                COPY_NORMAL_PS3(host_gamepad->rightStickX, report->rightStickX)
                COPY_NORMAL_PS3(host_gamepad->rightStickY, report->rightStickY)
                COPY_TRIGGER_PS3_XINPUT(host_gamepad->leftTrigger, report->leftTrigger)
                COPY_TRIGGER_PS3_XINPUT(host_gamepad->rightTrigger, report->rightTrigger)
            }
            break;
        }
        case PS4: {
            // Turn dpad back to bits so we can use them below
            PS4Gamepad_Data_t *host_gamepad = (PS4Gamepad_Data_t *)data;
            PS4Dpad_Data_t *dpad = (PS4Dpad_Data_t *)dpad;
            dpad->dpad = dpad->dpad > RIGHT ? 0 : dpad_bindings_reverse[dpad->dpad];
            COPY_BUTTON(host_gamepad->back, report->back)
            COPY_BUTTON(host_gamepad->start, report->start)
            COPY_BUTTON(host_gamepad->leftThumbClick, report->leftThumbClick)
            COPY_BUTTON(host_gamepad->rightThumbClick, report->rightThumbClick)
            COPY_BUTTON(host_gamepad->dpadUp, report->dpadUp)
            COPY_BUTTON(host_gamepad->dpadDown, report->dpadDown)
            COPY_BUTTON(host_gamepad->dpadLeft, report->dpadLeft)
            COPY_BUTTON(host_gamepad->dpadRight, report->dpadRight)
            COPY_BUTTON(host_gamepad->leftShoulder, report->leftShoulder)
            COPY_BUTTON(host_gamepad->rightShoulder, report->rightShoulder)

#ifdef HAS_L2_R2_BUTTON
            COPY_BUTTON(host_gamepad->l2, report->l2)
            COPY_BUTTON(host_gamepad->r2, report->r2)
#endif
            COPY_BUTTON(host_gamepad->x, report->x)
            COPY_BUTTON(host_gamepad->a, report->a)
            COPY_BUTTON(host_gamepad->b, report->b)
            COPY_BUTTON(host_gamepad->y, report->y)
            COPY_BUTTON(host_gamepad->guide, report->guide)
            COPY_NORMAL_PS3(host_gamepad->leftStickX, report->leftStickX)
            COPY_NORMAL_PS3(host_gamepad->leftStickY, report->leftStickY)
            COPY_NORMAL_PS3(host_gamepad->rightStickX, report->rightStickX)
            COPY_NORMAL_PS3(host_gamepad->rightStickY, report->rightStickY)
            COPY_TRIGGER_PS3_XINPUT(host_gamepad->leftTrigger, report->leftTrigger)
            COPY_TRIGGER_PS3_XINPUT(host_gamepad->rightTrigger, report->rightTrigger)
            break;
        }
        case WINDOWS_XBOX360: {
            XInputGamepad_Data_t *host_gamepad = (XInputGamepad_Data_t *)data;
            COPY_BUTTON(host_gamepad->back, report->back)
            COPY_BUTTON(host_gamepad->start, report->start)
            COPY_BUTTON(host_gamepad->leftThumbClick, report->leftThumbClick)
            COPY_BUTTON(host_gamepad->rightThumbClick, report->rightThumbClick)
            COPY_BUTTON(host_gamepad->dpadUp, report->dpadUp)
            COPY_BUTTON(host_gamepad->dpadDown, report->dpadDown)
            COPY_BUTTON(host_gamepad->dpadLeft, report->dpadLeft)
            COPY_BUTTON(host_gamepad->dpadRight, report->dpadRight)
            COPY_BUTTON(host_gamepad->leftShoulder, report->leftShoulder)
            COPY_BUTTON(host_gamepad->rightShoulder, report->rightShoulder)
            COPY_BUTTON(host_gamepad->x, report->x)
            COPY_BUTTON(host_gamepad->a, report->a)
            COPY_BUTTON(host_gamepad->b, report->b)
            COPY_BUTTON(host_gamepad->y, report->y)
            COPY_BUTTON(host_gamepad->guide, report->guide)
            COPY_AXIS_NORMAL(host_gamepad->leftStickX, report->leftStickX)
            COPY_AXIS_NORMAL(host_gamepad->leftStickY, report->leftStickY)
            COPY_AXIS_NORMAL(host_gamepad->rightStickX, report->rightStickX)
            COPY_AXIS_NORMAL(host_gamepad->rightStickY, report->rightStickY)
            COPY_TRIGGER_PS3_XINPUT(host_gamepad->leftTrigger, report->leftTrigger)
            COPY_TRIGGER_PS3_XINPUT(host_gamepad->rightTrigger, report->rightTrigger)
            break;
        }
        case XBOXONE: {
            XboxOneGamepad_Data_t *host_gamepad = (XboxOneGamepad_Data_t *)data;
            COPY_BUTTON(host_gamepad->back, report->back)
            COPY_BUTTON(host_gamepad->start, report->start)
            COPY_BUTTON(host_gamepad->leftThumbClick, report->leftThumbClick)
            COPY_BUTTON(host_gamepad->rightThumbClick, report->rightThumbClick)
            COPY_BUTTON(host_gamepad->dpadUp, report->dpadUp)
            COPY_BUTTON(host_gamepad->dpadDown, report->dpadDown)
            COPY_BUTTON(host_gamepad->dpadLeft, report->dpadLeft)
            COPY_BUTTON(host_gamepad->dpadRight, report->dpadRight)
            COPY_BUTTON(host_gamepad->leftShoulder, report->leftShoulder)
            COPY_BUTTON(host_gamepad->rightShoulder, report->rightShoulder)
            COPY_BUTTON(host_gamepad->x, report->x)
            COPY_BUTTON(host_gamepad->a, report->a)
            COPY_BUTTON(host_gamepad->b, report->b)
            COPY_BUTTON(host_gamepad->y, report->y)
            COPY_BUTTON(host_gamepad->guide, report->guide)
            COPY_AXIS_NORMAL(host_gamepad->leftStickX, report->leftStickX)
            COPY_AXIS_NORMAL(host_gamepad->leftStickY, report->leftStickY)
            COPY_AXIS_NORMAL(host_gamepad->rightStickX, report->rightStickX)
            COPY_AXIS_NORMAL(host_gamepad->rightStickY, report->rightStickY)
            COPY_AXIS_TRIGGER(host_gamepad->leftTrigger, report->leftTrigger)
            COPY_AXIS_TRIGGER(host_gamepad->rightTrigger, report->rightTrigger)
            break;
        }
    }

#elif DEVICE_TYPE_IS_LIVE_GUITAR
    switch (console_type) {
        case PS3: {
            if (device_type.sub_type == GAMEPAD) {
                PS3Gamepad_Data_t *host_gamepad = (PS3Gamepad_Data_t *)data;

                COPY_BUTTON(host_gamepad->dpadUp, report->dpadUp)
                COPY_BUTTON(host_gamepad->dpadDown, report->dpadDown)
                COPY_BUTTON(host_gamepad->dpadLeft, report->dpadLeft)
                COPY_BUTTON(host_gamepad->dpadRight, report->dpadRight)
                COPY_BUTTON(host_gamepad->x, report->white1)
                COPY_BUTTON(host_gamepad->a, report->black1)
                COPY_BUTTON(host_gamepad->b, report->black2)
                COPY_BUTTON(host_gamepad->y, report->black3)
                COPY_BUTTON(host_gamepad->leftShoulder, report->white2)
                COPY_BUTTON(host_gamepad->rightShoulder, report->back)
                COPY_BUTTON(host_gamepad->start, report->start)
                COPY_BUTTON(host_gamepad->leftThumbClick, report->leftThumbClick)
                COPY_BUTTON(host_gamepad->guide, report->guide)
                COPY_AXIS_NORMAL(host_gamepad->rightStickY, report->whammy)
            } else {
                PS3GHLGuitar_Data_t *host_gamepad = (PS3GHLGuitar_Data_t *)data;
                // Turn dpad back to bits so we can use them below
                PS3Dpad_Data_t *dpad = (PS3Dpad_Data_t *)dpad;
                dpad->dpad = dpad->dpad > RIGHT ? 0 : dpad_bindings_reverse[dpad->dpad];
                COPY_BUTTON(host_gamepad->dpadUp, report->dpadUp)
                COPY_BUTTON(host_gamepad->dpadDown, report->dpadDown)
                COPY_BUTTON(host_gamepad->dpadLeft, report->dpadLeft)
                COPY_BUTTON(host_gamepad->dpadRight, report->dpadRight)
                COPY_BUTTON(host_gamepad->white1, report->white1)
                COPY_BUTTON(host_gamepad->black1, report->black1)
                COPY_BUTTON(host_gamepad->black2, report->black2)
                COPY_BUTTON(host_gamepad->black3, report->black3)
                COPY_BUTTON(host_gamepad->white2, report->white2)
                COPY_BUTTON(host_gamepad->back, report->back)
                COPY_BUTTON(host_gamepad->start, report->start)
                COPY_BUTTON(host_gamepad->leftThumbClick, report->leftThumbClick)
                COPY_BUTTON(host_gamepad->guide, report->guide)
                COPY_AXIS_NORMAL(host_gamepad->whammy, report->whammy)
                COPY_TILT((host_gamepad->tilt >> 2) - 128)
            }
            break;
        }
        case PS4: {
            // Turn dpad back to bits so we can use them below
            PS4GHLGuitar_Data_t *host_gamepad = (PS4GHLGuitar_Data_t *)data;
            PS4Dpad_Data_t *dpad = (PS4Dpad_Data_t *)dpad;
            dpad->dpad = dpad->dpad > RIGHT ? 0 : dpad_bindings_reverse[dpad->dpad];
            COPY_BUTTON(host_gamepad->dpadUp, report->dpadUp)
            COPY_BUTTON(host_gamepad->dpadDown, report->dpadDown)
            COPY_BUTTON(host_gamepad->dpadLeft, report->dpadLeft)
            COPY_BUTTON(host_gamepad->dpadRight, report->dpadRight)
            COPY_BUTTON(host_gamepad->white1, report->white1)
            COPY_BUTTON(host_gamepad->black1, report->black1)
            COPY_BUTTON(host_gamepad->black2, report->black2)
            COPY_BUTTON(host_gamepad->black3, report->black3)
            COPY_BUTTON(host_gamepad->white2, report->white2)
            COPY_BUTTON(host_gamepad->back, report->back)
            COPY_BUTTON(host_gamepad->start, report->start)
            COPY_BUTTON(host_gamepad->leftThumbClick, report->leftThumbClick)
            COPY_BUTTON(host_gamepad->guide, report->guide)
            COPY_NORMAL_PS3(host_gamepad->whammy, report->whammy)
            COPY_TILT(host_gamepad->tilt)
            break;
        }
        case WINDOWS_XBOX360: {
            XInputGHLGuitar_Data_t *host_gamepad = (XInputGHLGuitar_Data_t *)data;
            COPY_BUTTON(host_gamepad->dpadUp, report->dpadUp)
            COPY_BUTTON(host_gamepad->dpadDown, report->dpadDown)
            COPY_BUTTON(host_gamepad->dpadLeft, report->dpadLeft)
            COPY_BUTTON(host_gamepad->dpadRight, report->dpadRight)
            COPY_BUTTON(host_gamepad->white1, report->white1)
            COPY_BUTTON(host_gamepad->black1, report->black1)
            COPY_BUTTON(host_gamepad->black2, report->black2)
            COPY_BUTTON(host_gamepad->black3, report->black3)
            COPY_BUTTON(host_gamepad->white2, report->white2)
            COPY_BUTTON(host_gamepad->back, report->back)
            COPY_BUTTON(host_gamepad->start, report->start)
            COPY_BUTTON(host_gamepad->leftThumbClick, report->leftThumbClick)
            COPY_BUTTON(host_gamepad->guide, report->guide)
            COPY_AXIS_NORMAL(host_gamepad->whammy, report->whammy)
            COPY_TILT((host_gamepad->tilt >> 8) + 128)
            break;
        }
        case XBOXONE: {
            // If we hit xb1 here, then the controller is in xb1 controller or is in xb1 compat mode
            // an actual guitar will instead take the PS3 path
            XboxOneGamepad_Data_t *host_gamepad = (XboxOneGamepad_Data_t *)data;

            COPY_BUTTON(host_gamepad->dpadUp, report->dpadUp)
            COPY_BUTTON(host_gamepad->dpadDown, report->dpadDown)
            COPY_BUTTON(host_gamepad->dpadLeft, report->dpadLeft)
            COPY_BUTTON(host_gamepad->dpadRight, report->dpadRight)
            COPY_BUTTON(host_gamepad->x, report->white1)
            COPY_BUTTON(host_gamepad->a, report->black1)
            COPY_BUTTON(host_gamepad->b, report->black2)
            COPY_BUTTON(host_gamepad->y, report->black3)
            COPY_BUTTON(host_gamepad->leftShoulder, report->white2)
            COPY_BUTTON(host_gamepad->rightShoulder, report->white3)
            COPY_BUTTON(host_gamepad->back, report->back)
            COPY_BUTTON(host_gamepad->start, report->start)
            COPY_BUTTON(host_gamepad->leftThumbClick, report->leftThumbClick)
            COPY_BUTTON(host_gamepad->guide, report->guide)
            COPY_AXIS_NORMAL(host_gamepad->rightStickY, report->whammy)
            break;
        }
    }
    int16_t strumBar = 0;
    if (report->dpadUp && !report->dpadDown) {
        strumBar = INT16_MIN;
    } else if (!report->dpadUp && report->dpadDown) {
        strumBar = INT16_MAX;
    }
    COPY_AXIS_NORMAL(strumBar, report->strumBar)
#elif DEVICE_TYPE == GUITAR
    switch (console_type) {
        case PS3: {
            if (device_type.sub_type == GAMEPAD) {
                PS3Gamepad_Data_t *host_gamepad = (PS3Gamepad_Data_t *)data;

                COPY_BUTTON(host_gamepad->back, report->back)
                COPY_BUTTON(host_gamepad->start, report->start)
                COPY_BUTTON(host_gamepad->dpadUp, report->dpadUp)
                COPY_BUTTON(host_gamepad->dpadDown, report->dpadDown)
                COPY_BUTTON(host_gamepad->dpadLeft, report->dpadLeft)
                COPY_BUTTON(host_gamepad->dpadRight, report->dpadRight)
                COPY_BUTTON(host_gamepad->leftShoulder, report->leftShoulder)
                COPY_BUTTON(host_gamepad->x, report->x)
                COPY_BUTTON(host_gamepad->a, report->a)
                COPY_BUTTON(host_gamepad->b, report->b)
                COPY_BUTTON(host_gamepad->y, report->y)
                COPY_BUTTON(host_gamepad->guide, report->guide)
                COPY_NORMAL_PS3(host_gamepad->rightStickX, report->whammy)
#if RHYTHM_TYPE == ROCK_BAND
                if (device_type.rhythm_type == GUITAR_HERO) {
                    COPY_BUTTON(host_gamepad->y, report->x)
                    COPY_BUTTON(host_gamepad->x, report->y)
                }
                COPY_NORMAL_PS3(host_gamepad->rightStickY, report->pickup)
#else
                if (device_type.rhythm_type == ROCK_BAND) {
                    COPY_BUTTON(host_gamepad->y, report->x)
                    COPY_BUTTON(host_gamepad->x, report->y)
                }
                COPY_NORMAL_PS3(host_gamepad->rightStickY, report->slider)
#endif
            } else {
                PS3RockBandGuitar_Data_t *host_gamepad = (PS3RockBandGuitar_Data_t *)data;
                // Turn dpad back to bits so we can use them below
                PS3Dpad_Data_t *dpad = (PS3Dpad_Data_t *)dpad;
                dpad->dpad = dpad->dpad > RIGHT ? 0 : dpad_bindings_reverse[dpad->dpad];
                COPY_BUTTON(host_gamepad->back, report->back)
                COPY_BUTTON(host_gamepad->start, report->start)
                COPY_BUTTON(host_gamepad->dpadUp, report->dpadUp)
                COPY_BUTTON(host_gamepad->dpadDown, report->dpadDown)
                COPY_BUTTON(host_gamepad->dpadLeft, report->dpadLeft)
                COPY_BUTTON(host_gamepad->dpadRight, report->dpadRight)
                COPY_BUTTON(host_gamepad->leftShoulder, report->leftShoulder)
                COPY_BUTTON(host_gamepad->x, report->x)
                COPY_BUTTON(host_gamepad->a, report->a)
                COPY_BUTTON(host_gamepad->b, report->b)
                COPY_BUTTON(host_gamepad->y, report->y)
                COPY_BUTTON(host_gamepad->guide, report->guide)
                COPY_NORMAL_PS3(host_gamepad->whammy, report->whammy)
                if (device_type.rhythm_type == GUITAR_HERO) {
                    PS3GuitarHeroGuitar_Data_t *host_gamepad2 = (PS3GuitarHeroGuitar_Data_t *)data;
                    COPY_TILT((host_gamepad2->tilt >> 2) - 128)
                } else {
                    COPY_TILT(host_gamepad->tilt ? 255 : 128)
                }
                // On ps3, rb and gh guitars swap their blue and yellow
#if RHYTHM_TYPE == ROCK_BAND
                if (device_type.rhythm_type == GUITAR_HERO) {
                    COPY_BUTTON(host_gamepad->y, report->x)
                    COPY_BUTTON(host_gamepad->x, report->y)
                } else {
                    COPY_BUTTON(host_gamepad->solo, report->solo)
#ifdef XB1_SOLO
                    if (host_gamepad->solo) {
                        COPY_BUTTON(host_gamepad->a, report->soloGreen)
                        COPY_BUTTON(host_gamepad->b, report->soloRed)
                        COPY_BUTTON(host_gamepad->x, report->soloYellow)
                        COPY_BUTTON(host_gamepad->y, report->soloBlue)
                        COPY_BUTTON(host_gamepad->leftShoulder, report->soloOrange)
                    } else {
                        COPY_BUTTON(host_gamepad->a, report->green)
                        COPY_BUTTON(host_gamepad->b, report->red)
                        COPY_BUTTON(host_gamepad->x, report->yellow)
                        COPY_BUTTON(host_gamepad->y, report->blue)
                        COPY_BUTTON(host_gamepad->leftShoulder, report->orange)
                    }
#endif
                }
                COPY_NORMAL_PS3(host_gamepad->pickup, report->pickup)
#else
                if (device_type.rhythm_type == ROCK_BAND) {
                    COPY_BUTTON(host_gamepad->y, report->x)
                    COPY_BUTTON(host_gamepad->x, report->y)
                }
                COPY_NORMAL_PS3(host_gamepad->pickup, report->slider)
#endif
            }
            break;
        }
        case PS4: {
            // Turn dpad back to bits so we can use them below
            PS4Gamepad_Data_t *host_gamepad = (PS4Gamepad_Data_t *)data;
            PS4Dpad_Data_t *dpad = (PS4Dpad_Data_t *)dpad;
            dpad->dpad = dpad->dpad > RIGHT ? 0 : dpad_bindings_reverse[dpad->dpad];
            COPY_BUTTON(host_gamepad->back, report->back)
            COPY_BUTTON(host_gamepad->start, report->start)
            COPY_BUTTON(host_gamepad->dpadUp, report->dpadUp)
            COPY_BUTTON(host_gamepad->dpadDown, report->dpadDown)
            COPY_BUTTON(host_gamepad->dpadLeft, report->dpadLeft)
            COPY_BUTTON(host_gamepad->dpadRight, report->dpadRight)
            COPY_BUTTON(host_gamepad->leftShoulder, report->leftShoulder)
            COPY_BUTTON(host_gamepad->x, report->x)
            COPY_BUTTON(host_gamepad->a, report->a)
            COPY_BUTTON(host_gamepad->b, report->b)
            COPY_BUTTON(host_gamepad->y, report->y)
            COPY_BUTTON(host_gamepad->guide, report->guide)
            COPY_NORMAL_PS3(host_gamepad->rightStickX, report->whammy)
#if RHYTHM_TYPE == ROCK_BAND
            COPY_NORMAL_PS3(host_gamepad->rightStickY, report->pickup)
#else
            COPY_NORMAL_PS3(host_gamepad->rightStickY, report->slider)
#endif
            break;
        }
        case WINDOWS_XBOX360: {
            XInputRockBandGuitar_Data_t *host_gamepad = (XInputRockBandGuitar_Data_t *)data;
            COPY_BUTTON(host_gamepad->back, report->back)
            COPY_BUTTON(host_gamepad->start, report->start)
            COPY_BUTTON(host_gamepad->dpadUp, report->dpadUp)
            COPY_BUTTON(host_gamepad->dpadDown, report->dpadDown)
            COPY_BUTTON(host_gamepad->dpadLeft, report->dpadLeft)
            COPY_BUTTON(host_gamepad->dpadRight, report->dpadRight)
            COPY_BUTTON(host_gamepad->leftShoulder, report->leftShoulder)
            COPY_BUTTON(host_gamepad->x, report->x)
            COPY_BUTTON(host_gamepad->a, report->a)
            COPY_BUTTON(host_gamepad->b, report->b)
            COPY_BUTTON(host_gamepad->y, report->y)
            COPY_BUTTON(host_gamepad->guide, report->guide)
            COPY_AXIS_NORMAL(host_gamepad->whammy, report->whammy)
            COPY_TILT((host_gamepad->tilt >> 8) + 128)
#if RHYTHM_TYPE == ROCK_BAND
            COPY_NORMAL_PS3(host_gamepad->pickup, report->pickup)
            COPY_BUTTON(host_gamepad->solo, report->solo)
#ifdef XB1_SOLO
            if (host_gamepad->solo) {
                COPY_BUTTON(host_gamepad->a, report->soloGreen)
                COPY_BUTTON(host_gamepad->b, report->soloRed)
                COPY_BUTTON(host_gamepad->x, report->soloYellow)
                COPY_BUTTON(host_gamepad->y, report->soloBlue)
                COPY_BUTTON(host_gamepad->leftShoulder, report->soloOrange)
            } else {
                COPY_BUTTON(host_gamepad->a, report->green)
                COPY_BUTTON(host_gamepad->b, report->red)
                COPY_BUTTON(host_gamepad->x, report->yellow)
                COPY_BUTTON(host_gamepad->y, report->blue)
                COPY_BUTTON(host_gamepad->leftShoulder, report->orange)
            }
#endif
#else
            COPY_NORMAL_PS3(host_gamepad->pickup, report->slider)
#endif
            break;
        }
        case XBOXONE: {
            XboxOneRockBandGuitar_Data_t *host_gamepad = (XboxOneRockBandGuitar_Data_t *)data;
            COPY_BUTTON(host_gamepad->back, report->back)
            COPY_BUTTON(host_gamepad->start, report->start)
            COPY_BUTTON(host_gamepad->dpadUp, report->dpadUp)
            COPY_BUTTON(host_gamepad->dpadDown, report->dpadDown)
            COPY_BUTTON(host_gamepad->dpadLeft, report->dpadLeft)
            COPY_BUTTON(host_gamepad->dpadRight, report->dpadRight)
            COPY_BUTTON(host_gamepad->leftShoulder, report->leftShoulder)
            COPY_BUTTON(host_gamepad->x, report->x)
            COPY_BUTTON(host_gamepad->a, report->a)
            COPY_BUTTON(host_gamepad->b, report->b)
            COPY_BUTTON(host_gamepad->y, report->y)
            COPY_BUTTON(host_gamepad->guide, report->guide)
#ifdef XB1_SOLO
            COPY_BUTTON(host_gamepad->green, report->green)
            COPY_BUTTON(host_gamepad->red, report->red)
            COPY_BUTTON(host_gamepad->yellow, report->yellow)
            COPY_BUTTON(host_gamepad->blue, report->blue)
            COPY_BUTTON(host_gamepad->orange, report->orange)
            COPY_BUTTON(host_gamepad->soloGreen, report->soloGreen)
            COPY_BUTTON(host_gamepad->soloRed, report->soloRed)
            COPY_BUTTON(host_gamepad->soloYellow, report->soloYellow)
            COPY_BUTTON(host_gamepad->soloBlue, report->soloBlue)
            COPY_BUTTON(host_gamepad->soloOrange, report->soloOrange)
#endif
            COPY_AXIS_NORMAL(host_gamepad->pickup, report->whammy)
            COPY_TILT(host_gamepad->tilt)
#if RHYTHM_TYPE == ROCK_BAND
            COPY_NORMAL_PS3(host_gamepad->pickup, report->pickup)
#else
            COPY_NORMAL_PS3(host_gamepad->pickup, report->slider)
#endif
            break;
        }
    }
#elif DEVICE_TYPE == DRUMS
    switch (console_type) {
        case PS3: {
            if (device_type.sub_type == GAMEPAD) {
                PS3Gamepad_Data_t *host_gamepad = (PS3Gamepad_Data_t *)data;

                COPY_BUTTON(host_gamepad->back, report->back)
                COPY_BUTTON(host_gamepad->start, report->start)
                COPY_BUTTON(host_gamepad->dpadUp, report->dpadUp)
                COPY_BUTTON(host_gamepad->dpadDown, report->dpadDown)
                COPY_BUTTON(host_gamepad->dpadLeft, report->dpadLeft)
                COPY_BUTTON(host_gamepad->dpadRight, report->dpadRight)
                COPY_BUTTON(host_gamepad->leftShoulder, report->leftShoulder)
                COPY_BUTTON(host_gamepad->x, report->x)
                COPY_BUTTON(host_gamepad->a, report->a)
                COPY_BUTTON(host_gamepad->b, report->b)
                COPY_BUTTON(host_gamepad->y, report->y)
                COPY_BUTTON(host_gamepad->guide, report->guide)
            } else {
                PS3GuitarHeroDrums_Data_t *host_gamepad = (PS3GuitarHeroDrums_Data_t *)data;
                // Turn dpad back to bits so we can use them below
                PS3Dpad_Data_t *dpad = (PS3Dpad_Data_t *)dpad;
                dpad->dpad = dpad->dpad > RIGHT ? 0 : dpad_bindings_reverse[dpad->dpad];
                COPY_BUTTON(host_gamepad->back, report->back)
                COPY_BUTTON(host_gamepad->start, report->start)
                COPY_BUTTON(host_gamepad->dpadUp, report->dpadUp)
                COPY_BUTTON(host_gamepad->dpadDown, report->dpadDown)
                COPY_BUTTON(host_gamepad->dpadLeft, report->dpadLeft)
                COPY_BUTTON(host_gamepad->dpadRight, report->dpadRight)
                COPY_BUTTON(host_gamepad->leftShoulder, report->leftShoulder)
                COPY_BUTTON(host_gamepad->x, report->x)
                COPY_BUTTON(host_gamepad->a, report->a)
                COPY_BUTTON(host_gamepad->b, report->b)
                COPY_BUTTON(host_gamepad->y, report->y)
                COPY_BUTTON(host_gamepad->guide, report->guide)
                if (device_type.sub_type == DRUMS) {
                    COPY_DRUM_VELOCITY_GREEN(host_gamepad->greenVelocity);
                    COPY_DRUM_VELOCITY_RED(host_gamepad->redVelocity);
                    COPY_DRUM_VELOCITY_YELLOW(host_gamepad->yellowVelocity);
                    COPY_DRUM_VELOCITY_BLUE(host_gamepad->blueVelocity);
#if RHYTHM_TYPE == GUITAR_HERO
                    if (device_type.rhythm_type == GUITAR_HERO) {
                        COPY_DRUM_VELOCITY_ORANGE(host_gamepad->orangeVelocity);
                        COPY_DRUM_VELOCITY_KICK(host_gamepad->kickVelocity);
                    }
                }
#endif
            }
            break;
        }
        case PS4: {
            // Turn dpad back to bits so we can use them below
            PS4Gamepad_Data_t *host_gamepad = (PS4Gamepad_Data_t *)data;
            PS4Dpad_Data_t *dpad = (PS4Dpad_Data_t *)dpad;
            dpad->dpad = dpad->dpad > RIGHT ? 0 : dpad_bindings_reverse[dpad->dpad];
            COPY_BUTTON(host_gamepad->back, report->back)
            COPY_BUTTON(host_gamepad->start, report->start)
            COPY_BUTTON(host_gamepad->dpadUp, report->dpadUp)
            COPY_BUTTON(host_gamepad->dpadDown, report->dpadDown)
            COPY_BUTTON(host_gamepad->dpadLeft, report->dpadLeft)
            COPY_BUTTON(host_gamepad->dpadRight, report->dpadRight)
            COPY_BUTTON(host_gamepad->leftShoulder, report->leftShoulder)
            COPY_BUTTON(host_gamepad->x, report->x)
            COPY_BUTTON(host_gamepad->a, report->a)
            COPY_BUTTON(host_gamepad->b, report->b)
            COPY_BUTTON(host_gamepad->y, report->y)
            COPY_BUTTON(host_gamepad->guide, report->guide)
            break;
        }
        case WINDOWS_XBOX360: {
            XInputGuitarHeroDrums_Data_t *host_gamepad = (XInputGuitarHeroDrums_Data_t *)data;
            COPY_BUTTON(host_gamepad->back, report->back)
            COPY_BUTTON(host_gamepad->start, report->start)
            COPY_BUTTON(host_gamepad->dpadUp, report->dpadUp)
            COPY_BUTTON(host_gamepad->dpadDown, report->dpadDown)
            COPY_BUTTON(host_gamepad->dpadLeft, report->dpadLeft)
            COPY_BUTTON(host_gamepad->dpadRight, report->dpadRight)
            COPY_BUTTON(host_gamepad->leftShoulder, report->leftShoulder)
            COPY_BUTTON(host_gamepad->x, report->x)
            COPY_BUTTON(host_gamepad->a, report->a)
            COPY_BUTTON(host_gamepad->b, report->b)
            COPY_BUTTON(host_gamepad->y, report->y)
            COPY_BUTTON(host_gamepad->guide, report->guide)
            if (device_type.sub_type == DRUMS) {
                if (device_type.rhythm_type == GUITAR_HERO) {
    #if RHYTHM_TYPE == GUITAR_HERO
                    COPY_DRUM_VELOCITY_GREEN(host_gamepad->greenVelocity);
                    COPY_DRUM_VELOCITY_RED(host_gamepad->redVelocity);
                    COPY_DRUM_VELOCITY_YELLOW(host_gamepad->yellowVelocity);
                    COPY_DRUM_VELOCITY_BLUE(host_gamepad->blueVelocity);
                    COPY_DRUM_VELOCITY_ORANGE(host_gamepad->orangeVelocity);
                    COPY_DRUM_VELOCITY_KICK(host_gamepad->kickVelocity);
    #endif
                } else {
                    XInputRockBandDrums_Data_t *host_gamepad2 = (XInputRockBandDrums_Data_t *)data;

                    COPY_DRUM_VELOCITY_GREEN((-(0x7fff - host_gamepad2->greenVelocity)) >> 8);
                    COPY_DRUM_VELOCITY_YELLOW(-(0x7fff - host_gamepad2->yellowVelocity) >> 8);
                    COPY_DRUM_VELOCITY_RED((0x7fff - host_gamepad2->redVelocity) >> 8);
                    COPY_DRUM_VELOCITY_BLUE((0x7fff - host_gamepad2->blueVelocity) >> 8);
                }
            }
            break;
        }
        case XBOXONE: {
            XboxOneRockBandDrums_Data_t *host_gamepad = (XboxOneRockBandDrums_Data_t *)data;
            COPY_BUTTON(host_gamepad->back, report->back)
            COPY_BUTTON(host_gamepad->start, report->start)
            COPY_BUTTON(host_gamepad->dpadUp, report->dpadUp)
            COPY_BUTTON(host_gamepad->dpadDown, report->dpadDown)
            COPY_BUTTON(host_gamepad->dpadLeft, report->dpadLeft)
            COPY_BUTTON(host_gamepad->dpadRight, report->dpadRight)
            COPY_BUTTON(host_gamepad->leftShoulder, report->leftShoulder)
            COPY_BUTTON(host_gamepad->x, report->x)
            COPY_BUTTON(host_gamepad->a, report->a)
            COPY_BUTTON(host_gamepad->b, report->b)
            COPY_BUTTON(host_gamepad->y, report->y)
            COPY_BUTTON(host_gamepad->guide, report->guide)
            if (device_type.sub_type == DRUMS) {
                COPY_DRUM_VELOCITY_GREEN(host_gamepad->greenVelocity);
                COPY_DRUM_VELOCITY_YELLOW(host_gamepad->yellowVelocity);
                COPY_DRUM_VELOCITY_RED(host_gamepad->redVelocity);
                COPY_DRUM_VELOCITY_BLUE(host_gamepad->blueVelocity);
                COPY_DRUM_VELOCITY_GREEN_CYMBAL(host_gamepad->greenCymbalVelocity);
                COPY_DRUM_VELOCITY_YELLOW_CYMBAL(host_gamepad->yellowCymbalVelocity);
                COPY_DRUM_VELOCITY_BLUE_CYMBAL(host_gamepad->blueCymbalVelocity);
            }
            break;
        }
    }

#elif DEVICE_TYPE == DJ_HERO_TURNTABLE
    switch (console_type) {
        case PS3: {
            if (device_type.sub_type == GAMEPAD) {
                PS3Gamepad_Data_t *host_gamepad = (PS3Gamepad_Data_t *)data;

                COPY_BUTTON(host_gamepad->back, report->back)
                COPY_BUTTON(host_gamepad->start, report->start)
                COPY_BUTTON(host_gamepad->dpadUp, report->dpadUp)
                COPY_BUTTON(host_gamepad->dpadDown, report->dpadDown)
                COPY_BUTTON(host_gamepad->dpadLeft, report->dpadLeft)
                COPY_BUTTON(host_gamepad->dpadRight, report->dpadRight)
                COPY_BUTTON(host_gamepad->x, report->x)
                COPY_BUTTON(host_gamepad->a, report->a)
                COPY_BUTTON(host_gamepad->b, report->b)
                COPY_BUTTON(host_gamepad->y, report->y)
                COPY_BUTTON(host_gamepad->x, report->leftBlue)
                COPY_BUTTON(host_gamepad->a, report->leftGreen)
                COPY_BUTTON(host_gamepad->b, report->leftRed)
                COPY_BUTTON(host_gamepad->x, report->rightBlue)
                COPY_BUTTON(host_gamepad->a, report->rightGreen)
                COPY_BUTTON(host_gamepad->b, report->rightRed)
                COPY_BUTTON(host_gamepad->guide, report->guide)
            } else {
                PS3Turntable_Data_t *host_gamepad = (PS3Turntable_Data_t *)data;
                // Turn dpad back to bits so we can use them below
                PS3Dpad_Data_t *dpad = (PS3Dpad_Data_t *)dpad;
                dpad->dpad = dpad->dpad > RIGHT ? 0 : dpad_bindings_reverse[dpad->dpad];
                COPY_BUTTON(host_gamepad->back, report->back)
                COPY_BUTTON(host_gamepad->start, report->start)
                COPY_BUTTON(host_gamepad->dpadUp, report->dpadUp)
                COPY_BUTTON(host_gamepad->dpadDown, report->dpadDown)
                COPY_BUTTON(host_gamepad->dpadLeft, report->dpadLeft)
                COPY_BUTTON(host_gamepad->dpadRight, report->dpadRight)
                COPY_BUTTON(host_gamepad->x, report->x)
                COPY_BUTTON(host_gamepad->a, report->a)
                COPY_BUTTON(host_gamepad->b, report->b)
                COPY_BUTTON(host_gamepad->y, report->y)
                COPY_BUTTON(host_gamepad->leftBlue, report->leftBlue)
                COPY_BUTTON(host_gamepad->leftGreen, report->leftGreen)
                COPY_BUTTON(host_gamepad->leftRed, report->leftRed)
                COPY_BUTTON(host_gamepad->rightBlue, report->rightBlue)
                COPY_BUTTON(host_gamepad->rightGreen, report->rightGreen)
                COPY_BUTTON(host_gamepad->rightRed, report->rightRed)
                COPY_BUTTON(host_gamepad->guide, report->guide)
                COPY_AXIS_DJ(host_gamepad->effectsKnob, report->effectsKnob)
                COPY_AXIS_DJ(host_gamepad->crossfader, report->crossfader)
            }
            break;
        }
        case PS4: {
            // Turn dpad back to bits so we can use them below
            PS4Gamepad_Data_t *host_gamepad = (PS4Gamepad_Data_t *)data;
            PS4Dpad_Data_t *dpad = (PS4Dpad_Data_t *)dpad;
            dpad->dpad = dpad->dpad > RIGHT ? 0 : dpad_bindings_reverse[dpad->dpad];
            COPY_BUTTON(host_gamepad->back, report->back)
            COPY_BUTTON(host_gamepad->start, report->start)
            COPY_BUTTON(host_gamepad->dpadUp, report->dpadUp)
            COPY_BUTTON(host_gamepad->dpadDown, report->dpadDown)
            COPY_BUTTON(host_gamepad->dpadLeft, report->dpadLeft)
            COPY_BUTTON(host_gamepad->dpadRight, report->dpadRight)
            COPY_BUTTON(host_gamepad->x, report->x)
            COPY_BUTTON(host_gamepad->a, report->a)
            COPY_BUTTON(host_gamepad->b, report->b)
            COPY_BUTTON(host_gamepad->y, report->y)
            COPY_BUTTON(host_gamepad->x, report->leftBlue)
            COPY_BUTTON(host_gamepad->a, report->leftGreen)
            COPY_BUTTON(host_gamepad->b, report->leftRed)
            COPY_BUTTON(host_gamepad->x, report->rightBlue)
            COPY_BUTTON(host_gamepad->a, report->rightGreen)
            COPY_BUTTON(host_gamepad->b, report->rightRed)
            COPY_BUTTON(host_gamepad->guide, report->guide)
            break;
        }
        case WINDOWS_XBOX360: {
            XInputTurntable_Data_t *host_gamepad = (XInputTurntable_Data_t *)data;
            COPY_BUTTON(host_gamepad->back, report->back)
            COPY_BUTTON(host_gamepad->start, report->start)
            COPY_BUTTON(host_gamepad->dpadUp, report->dpadUp)
            COPY_BUTTON(host_gamepad->dpadDown, report->dpadDown)
            COPY_BUTTON(host_gamepad->dpadLeft, report->dpadLeft)
            COPY_BUTTON(host_gamepad->dpadRight, report->dpadRight)
            COPY_BUTTON(host_gamepad->x, report->x)
            COPY_BUTTON(host_gamepad->a, report->a)
            COPY_BUTTON(host_gamepad->b, report->b)
            COPY_BUTTON(host_gamepad->y, report->y)
            COPY_BUTTON(host_gamepad->leftBlue, report->leftBlue)
            COPY_BUTTON(host_gamepad->leftGreen, report->leftGreen)
            COPY_BUTTON(host_gamepad->leftRed, report->leftRed)
            COPY_BUTTON(host_gamepad->rightBlue, report->rightBlue)
            COPY_BUTTON(host_gamepad->rightGreen, report->rightGreen)
            COPY_BUTTON(host_gamepad->rightRed, report->rightRed)
            COPY_BUTTON(host_gamepad->guide, report->guide)
            COPY_AXIS_DJ(host_gamepad->effectsKnob, report->effectsKnob)
            COPY_AXIS_DJ(host_gamepad->crossfader, report->crossfader)
            break;
        }
        case XBOXONE: {
            XboxOneGamepad_Data_t *host_gamepad = (XboxOneGamepad_Data_t *)data;
            COPY_BUTTON(host_gamepad->back, report->back)
            COPY_BUTTON(host_gamepad->start, report->start)
            COPY_BUTTON(host_gamepad->dpadUp, report->dpadUp)
            COPY_BUTTON(host_gamepad->dpadDown, report->dpadDown)
            COPY_BUTTON(host_gamepad->dpadLeft, report->dpadLeft)
            COPY_BUTTON(host_gamepad->dpadRight, report->dpadRight)
            COPY_BUTTON(host_gamepad->x, report->x)
            COPY_BUTTON(host_gamepad->a, report->a)
            COPY_BUTTON(host_gamepad->b, report->b)
            COPY_BUTTON(host_gamepad->y, report->y)
            COPY_BUTTON(host_gamepad->x, report->leftBlue)
            COPY_BUTTON(host_gamepad->a, report->leftGreen)
            COPY_BUTTON(host_gamepad->b, report->leftRed)
            COPY_BUTTON(host_gamepad->x, report->rightBlue)
            COPY_BUTTON(host_gamepad->a, report->rightGreen)
            COPY_BUTTON(host_gamepad->b, report->rightRed)
            COPY_BUTTON(host_gamepad->guide, report->guide)
            break;
        }
    }
#endif
}
#endif