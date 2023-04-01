// TODO: before calling COPY_AXIS_NORMAL, change the value to uint16_t
// TODO: before calling COPY_AXIS_TRIGGER, change the value to int16_t
#define COPY_NORMAL_PS3(in, out) COPY_AXIS_NORMAL(((in + 128) << 8), out)
#define COPY_TRIGGER_PS3_XINPUT(in, out) COPY_AXIS_TRIGGER(in << 8, out)
#ifdef INPUT_USB_HOST

for (int i = 0; i < device_count; i++) {
    USB_Device_Type_t device_type = get_usb_host_device_type(i);
    get_usb_host_device_data(i, (uint8_t *)&temp_report_usb_host);
    void *data = &temp_report_usb_host;
    if (device_type.console_type == XBOXONE) {
        GipHeader_t *header = (GipHeader_t *)data;
        if (device_type.sub_type == LIVE_GUITAR && header->command == GHL_HID_REPORT) {
            // Xbox one GHL guitars actually end up using PS3 reports if you poke them.
            device_type.console_type = PS3;
            data = &((XboxOneGHLGuitar_Data_t *)data)->report;
        } else if (header->command != GIP_INPUT_REPORT) {
            // Not input data, continue
            continue;
        }
    }
#if !DEVICE_TYPE_IS_INSTRUMENT
    switch (device_type.console_type) {
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
    switch (device_type.console_type) {
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
                COPY_BUTTON(host_gamepad->rightShoulder, report->heroPower)
                COPY_BUTTON(host_gamepad->start, report->pause)
                COPY_BUTTON(host_gamepad->leftThumbClick, report->ghtv)
                COPY_BUTTON(host_gamepad->guide, report->guide)
                COPY_AXIS_NORMAL(host_gamepad->rightStickY, report->whammy)
                // TODO: tilt
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
                COPY_BUTTON(host_gamepad->heroPower, report->heroPower)
                COPY_BUTTON(host_gamepad->pause, report->pause)
                COPY_BUTTON(host_gamepad->ghtv, report->ghtv)
                COPY_BUTTON(host_gamepad->guide, report->guide)
                COPY_AXIS_NORMAL(host_gamepad->whammy, report->whammy)
                // TODO: tilt
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
            COPY_BUTTON(host_gamepad->heroPower, report->heroPower)
            COPY_BUTTON(host_gamepad->pause, report->pause)
            COPY_BUTTON(host_gamepad->ghtv, report->ghtv)
            COPY_BUTTON(host_gamepad->guide, report->guide)
            COPY_NORMAL_PS3(host_gamepad->whammy, report->whammy)
            // TODO: tilt
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
            COPY_BUTTON(host_gamepad->heroPower, report->heroPower)
            COPY_BUTTON(host_gamepad->pause, report->pause)
            COPY_BUTTON(host_gamepad->ghtv, report->ghtv)
            COPY_BUTTON(host_gamepad->guide, report->guide)
            COPY_AXIS_NORMAL(host_gamepad->whammy, report->whammy)
            // TODO: tilt
            break;
        }
        case XBOXONE: {
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
            COPY_BUTTON(host_gamepad->rightShoulder, report->heroPower)
            COPY_BUTTON(host_gamepad->start, report->pause)
            COPY_BUTTON(host_gamepad->leftThumbClick, report->ghtv)
            COPY_BUTTON(host_gamepad->guide, report->guide)
            COPY_AXIS_NORMAL(host_gamepad->rightStickY, report->whammy)
            // TODO: tilt
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
    switch (device_type.console_type) {
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
                // TODO solo
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
                PS3GuitarHeroGuitar_Data_t *host_gamepad = (PS3GuitarHeroGuitar_Data_t *)data;
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
                // On ps3, rb and gh guitars swap their blue and yellow
                // TODO tilt
#if RHYTHM_TYPE == ROCK_BAND
                // TODO solo
                if (device_type.rhythm_type == GUITAR_HERO) {
                    COPY_BUTTON(host_gamepad->y, report->x)
                    COPY_BUTTON(host_gamepad->x, report->y)
                }
                COPY_NORMAL_PS3(host_gamepad->slider, report->pickup)
#else
                if (device_type.rhythm_type == ROCK_BAND) {
                    COPY_BUTTON(host_gamepad->y, report->x)
                    COPY_BUTTON(host_gamepad->x, report->y)
                }
                COPY_NORMAL_PS3(host_gamepad->slider, report->slider)
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
// TODO:  tilt, solo
#if RHYTHM_TYPE == ROCK_BAND
            COPY_NORMAL_PS3(host_gamepad->rightStickY, report->pickup)
#else
            COPY_NORMAL_PS3(host_gamepad->rightStickY, report->slider)
#endif
            break;
        }
        case WINDOWS_XBOX360: {
            XInputGuitarHeroGuitar_Data_t *host_gamepad = (XInputGuitarHeroGuitar_Data_t *)data;
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
// TODO:  tilt, solo
#if RHYTHM_TYPE == ROCK_BAND
            COPY_NORMAL_PS3(host_gamepad->slider, report->pickup)
#else
            COPY_NORMAL_PS3(host_gamepad->slider, report->slider)
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
            COPY_AXIS_NORMAL(host_gamepad->pickup, report->whammy)
// TODO:  tilt, solo
#if RHYTHM_TYPE == ROCK_BAND
            COPY_NORMAL_PS3(host_gamepad->pickup, report->pickup)
#else
            COPY_NORMAL_PS3(host_gamepad->pickup, report->slider)
#endif
            break;
        }
    }
#elif DEVICE_TYPE == DRUMS
    switch (device_type.console_type) {
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
                PS3GuitarHeroGuitar_Data_t *host_gamepad = (PS3GuitarHeroGuitar_Data_t *)data;
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
                // TODO: velocity
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
            // TODO: velocity
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
            // TODO: velocity
            break;
        }
    }

#elif DEVICE_TYPE == DJ_HERO_TURNTABLE
    switch (device_type.console_type) {
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
                // TODO: effectsKnob + crossFader
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
            // TODO: effectsKnob + crossFader
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
            // TODO: effectsKnob + crossFader
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
            // TODO: effectsKnob + crossFader
            break;
        }
    }
#endif
}
#endif