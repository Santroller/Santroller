// For whatever reason, the acceleration for guitars swings between -128 to 128, not -512 to 512
// Also, the game is looking for the value going negative, not positive
#define PS3_GH_TILT(val) PS3_ACCEL_CENTER + ((-(val >> 7)) - GUITAR_ONE_G)
#define PS3_GH_TILT_REV(val) (-((val - PS3_ACCEL_CENTER) + GUITAR_ONE_G)) << 7
#define XBOX_WHAMMY(val) (val << 8) - INT16_MAX
#define XBOX_WHAMMY_REV(val) (val + INT16_MAX) >> 8
#define XBOX_SLIDER(val) -((int8_t)((val) ^ 0x80) * -257)
#define XBOX_SLIDER_REV(val) usb_host_data->slider

// PS3 GH whammy ignores the negative half of the axis, so shift to get between 0 and 127, then add center
#define PS3_GH_WHAMMY(val) (val >> 1) + PS3_STICK_CENTER
#define PS3_GH_WHAMMY_REV(val) (val >> 1) + PS3_STICK_CENTER
#define PS2_GH_WHAMMY(val) 0x7f - (val >> 1)
#define PS2_GH_TILT(val) val > 20000
#define PS2_GH_TILT_REV(val) val ? INT16_MAX : 0

#define TRANSLATE_GHL_GUITAR(TILT, WHAMMY, SRC, DEST) \
    DEST->black1 |= SRC->black1;                      \
    DEST->black2 |= SRC->black2;                      \
    DEST->black3 |= SRC->black3;                      \
    DEST->white1 |= SRC->white1;                      \
    DEST->white2 |= SRC->white2;                      \
    DEST->white3 |= SRC->white3;                      \
    DEST->ghtv |= SRC->ghtv;                          \
    if (SRC->dpadUp) {                                \
        DEST->leftStickY = 0x00;                      \
    }                                                 \
    if (SRC->dpadDown) {                              \
        DEST->leftStickY = 0xFF;                      \
    }                                                 \
    if (SRC->tilt) {                                  \
        DEST->tilt = TILT(SRC->tilt);                 \
    }                                                 \
    if (SRC->whammy) {                                \
        DEST->whammy = WHAMMY(SRC->whammy);           \
    }
#define TRANSLATE_GH_GUITAR_SLIDER_REV(SLIDER, wt)  \
    if (wt) { \
        if (SLIDER < 0x2F) {                        \
            usb_host_data->slider = 0x15;           \
        } else if (SLIDER <= 0x3F) {                \
            usb_host_data->slider = 0x30;           \
        } else if (SLIDER <= 0x5F) {                \
            usb_host_data->slider = 0x4D;           \
        } else if (SLIDER <= 0x6F) {                \
            usb_host_data->slider = 0x66;           \
        } else if (SLIDER <= 0x8F) {                \
            usb_host_data->slider = 0x80;           \
        } else if (SLIDER <= 0x9F) {                \
            usb_host_data->slider = 0x9A;           \
        } else if (SLIDER <= 0xAF) {                \
            usb_host_data->slider = 0xAF;           \
        } else if (SLIDER <= 0xCF) {                \
            usb_host_data->slider = 0xC9;           \
        } else if (SLIDER <= 0xEF) {                \
            usb_host_data->slider = 0xE6;           \
        } else {                                    \
            usb_host_data->slider = 0xFF;           \
        }                                           \
    }
#define TRANSLATE_GH_GUITAR(TILT, WHAMMY, SLIDER, JOYSTICK, SRC, DEST) \
    DEST->green |= SRC->green;                                         \
    DEST->red |= SRC->red;                                             \
    DEST->yellow |= SRC->yellow;                                       \
    DEST->blue |= SRC->blue;                                           \
    DEST->orange |= SRC->orange;                                       \
    DEST->pedal |= SRC->pedal;                                         \
    if (SRC->tilt) {                                                   \
        DEST->tilt = TILT(SRC->tilt);                                  \
    }                                                                  \
    if (SRC->whammy) {                                                 \
        DEST->whammy = WHAMMY(SRC->whammy);                            \
    }                                                                  \
    if (SRC->leftStickX) {                                             \
        DEST->leftStickX = JOYSTICK(SRC->leftStickX);                  \
    }                                                                  \
    if (SRC->leftStickY) {                                             \
        DEST->leftStickY = JOYSTICK(-SRC->leftStickY);                 \
    }                                                                  \
    if (SRC->slider) {                                                 \
        DEST->slider = SLIDER(SRC->slider);                            \
    }
#define TRANSLATE_RB_GUITAR_COMBINED_SOLO_REV(SRC, DEST) \
    DEST->green |= SRC->soloGreen;                       \
    DEST->red |= SRC->soloRed;                           \
    DEST->yellow |= SRC->soloYellow;                     \
    DEST->blue |= SRC->soloBlue;                         \
    DEST->orange |= SRC->soloOrange;                     \
    DEST->solo |= SRC->soloGreen || SRC->soloRed || SRC->soloYellow || SRC->soloBlue || SRC->soloOrange;

#define TRANSLATE_RB_GUITAR_COMBINED_SOLO(SRC, DEST) \
    if (SRC->solo) {                                 \
        DEST->soloGreen |= SRC->green;               \
        DEST->soloRed |= SRC->red;                   \
        DEST->soloYellow |= SRC->yellow;             \
        DEST->soloBlue |= SRC->blue;                 \
        DEST->soloOrange |= SRC->orange;             \
    }

#define TRANSLATE_RB_GUITAR_SPLIT_SOLO(SRC, DEST) \
    DEST->soloGreen |= SRC->soloGreen;            \
    DEST->soloRed |= SRC->soloRed;                \
    DEST->soloYellow |= SRC->soloYellow;          \
    DEST->soloBlue |= SRC->soloBlue;              \
    DEST->soloOrange |= SRC->soloOrange;

#define TRANSLATE_RB_GUITAR(TILT, WHAMMY, PICKUP, JOYSTICK, SRC, DEST) \
    DEST->green |= SRC->green;                                         \
    DEST->red |= SRC->red;                                             \
    DEST->yellow |= SRC->yellow;                                       \
    DEST->blue |= SRC->blue;                                           \
    DEST->orange |= SRC->orange;                                       \
    if (SRC->tilt) {                                                   \
        DEST->tilt = TILT(SRC->tilt);                                  \
    }                                                                  \
    if (SRC->whammy) {                                                 \
        DEST->whammy = WHAMMY(SRC->whammy);                            \
    }                                                                  \
    if (SRC->leftStickX) {                                             \
        DEST->leftStickX = JOYSTICK(SRC->leftStickX);                  \
    }                                                                  \
    if (SRC->leftStickY) {                                             \
        DEST->leftStickY = JOYSTICK(-SRC->leftStickY);                 \
    }                                                                  \
    if (SRC->pickup) {                                                 \
        DEST->pickup = PICKUP(SRC->pickup);                            \
    }
