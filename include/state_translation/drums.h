#define XINPUT_RB_DRUM_GREEN_YELLOW(val) -((0x7FFF - val) >> 7)
#define XINPUT_RB_DRUM_RED_BLUE(val) (0x7FFF - val) >> 7
#define XINPUT_RB_DRUM_GREEN_YELLOW_REV(val) -((0x7FFF - (val << 7)))
#define XINPUT_RB_DRUM_RED_BLUE_REV(val) (0x7FFF - (val << 7))
#define PS3_RB_DRUM(val) 0xFF - val
#define XB1_RB_DRUM(val) val >> 4
#define XB1_RB_DRUM_REV(val) val << 4
#define TRANSLATE_GH_DRUMS(VELOCITY, SRC, DEST)     \
    DEST->green |= SRC->green;                      \
    DEST->red |= SRC->red;                          \
    DEST->yellow |= SRC->yellow;                    \
    DEST->blue |= SRC->blue;                        \
    DEST->orange |= SRC->orange;                    \
    DEST->kick1 |= SRC->kick1;                      \
    if (SRC->greenVelocity) {                       \
        DEST->greenVelocity = SRC->greenVelocity;   \
    }                                               \
    if (SRC->redVelocity) {                         \
        DEST->redVelocity = SRC->redVelocity;       \
    }                                               \
    if (SRC->yellowVelocity) {                      \
        DEST->yellowVelocity = SRC->yellowVelocity; \
    }                                               \
    if (SRC->blueVelocity) {                        \
        DEST->blueVelocity = SRC->blueVelocity;     \
    }                                               \
    if (SRC->orangeVelocity) {                      \
        DEST->orangeVelocity = SRC->orangeVelocity; \
    }                                               \
    if (SRC->kickVelocity) {                        \
        DEST->kickVelocity = SRC->kickVelocity;     \
    }

#define TRANSLATE_RB_DRUMS_COMBINED(VELOCITY, SRC, DEST)                \
    DEST->green |= SRC->green;                                          \
    DEST->red |= SRC->red;                                              \
    DEST->yellow |= SRC->yellow;                                        \
    DEST->blue |= SRC->blue;                                            \
    DEST->kick1 |= SRC->kick1;                                          \
    DEST->kick2 |= SRC->kick2;                                          \
    if (SRC->greenVelocity) {                                           \
        DEST->greenVelocity = VELOCITY(SRC->greenVelocity);             \
    }                                                                   \
    if (DEST->greenCymbalVelocity) {                                    \
        DEST->greenCymbalVelocity = VELOCITY(SRC->greenCymbalVelocity); \
    }                                                                   \
    if (SRC->redVelocity) {                                             \
        DEST->redVelocity = VELOCITY(SRC->redVelocity);                 \
    }                                                                   \
    if (SRC->yellowVelocity) {                                          \
        DEST->yellowVelocity = VELOCITY(SRC->yellowVelocity);           \
    }                                                                   \
    if (DEST->yellowCymbalVelocity) {                                   \
        DEST->yellowCymbalVelocity = VELOCITY(SRC->yellowVelocity);     \
    }                                                                   \
    if (SRC->blueVelocity) {                                            \
        DEST->blueVelocity = VELOCITY(SRC->blueVelocity);               \
    }                                                                   \
    if (DEST->blueCymbalVelocity) {                                     \
        DEST->blueCymbalVelocity = VELOCITY(SRC->blueVelocity);         \
    }

#define TRANSLATE_RB_DRUMS_SPLIT(GREEN, RED, YELLOW, BLUE, SRC, DEST) \
    SET_PADS();                                                       \
    DEST->green |= SRC->green;                                        \
    DEST->red |= SRC->red;                                            \
    DEST->yellow |= SRC->yellow;                                      \
    DEST->blue |= SRC->blue;                                          \
    DEST->kick1 |= SRC->kick1;                                        \
    DEST->kick2 |= SRC->kick2;                                        \
    if (SRC->greenVelocity) {                                         \
        if (DEST->greenCymbal) {                                      \
            DEST->greenCymbalVelocity = GREEN(SRC->greenVelocity);    \
        } else {                                                      \
            DEST->greenVelocity = GREEN(SRC->greenVelocity);          \
        }                                                             \
    }                                                                 \
    if (SRC->redVelocity) {                                           \
        DEST->redVelocity = RED(SRC->redVelocity);                    \
    }                                                                 \
    if (SRC->yellowVelocity) {                                        \
        if (DEST->yellowCymbal) {                                     \
            DEST->yellowCymbalVelocity = YELLOW(SRC->yellowVelocity); \
        } else {                                                      \
            DEST->yellowVelocity = YELLOW(SRC->yellowVelocity);       \
        }                                                             \
    }                                                                 \
    if (SRC->blueVelocity) {                                          \
        if (DEST->blueCymbal) {                                       \
            DEST->blueCymbalVelocity = BLUE(SRC->blueVelocity);       \
        } else {                                                      \
            DEST->blueVelocity = BLUE(SRC->blueVelocity);             \
        }                                                             \
    }

#define TRANSLATE_RB_DRUMS_SPLIT_REV(GREEN, RED, YELLOW, BLUE, SRC, DEST)         \
    DEST->green |= SRC->green;                                                    \
    DEST->red |= SRC->red;                                                        \
    DEST->yellow |= SRC->yellow;                                                  \
    DEST->blue |= SRC->blue;                                                      \
    DEST->kick1 |= SRC->kick1;                                                    \
    DEST->kick2 |= SRC->kick2;                                                    \
    DEST->padFlag |= SRC->green || SRC->red || SRC->yellow || SRC->blue;          \
    DEST->cymbalFlag |= SRC->greenCymbal || SRC->yellowCymbal || SRC->blueCymbal; \
    if (SRC->greenVelocity) {                                                     \
        if (SRC->greenCymbalVelocity) {                                           \
            DEST->greenVelocity = GREEN(SRC->greenCymbalVelocity);                \
        } else {                                                                  \
            DEST->greenVelocity = GREEN(SRC->greenVelocity);                      \
        }                                                                         \
    }                                                                             \
    if (SRC->redVelocity) {                                                       \
        DEST->redVelocity = RED(SRC->redVelocity);                                \
    }                                                                             \
    if (SRC->yellowVelocity) {                                                    \
        if (SRC->yellowCymbalVelocity) {                                          \
            DEST->yellowVelocity = YELLOW(SRC->yellowCymbalVelocity);             \
        } else {                                                                  \
            DEST->yellowVelocity = YELLOW(SRC->yellowVelocity);                   \
        }                                                                         \
    }                                                                             \
    if (SRC->blueVelocity) {                                                      \
        if (SRC->blueCymbalVelocity) {                                            \
            DEST->blueVelocity = BLUE(SRC->blueCymbalVelocity);                   \
        } else {                                                                  \
            DEST->blueVelocity = BLUE(SRC->blueVelocity);                         \
        }                                                                         \
    }
