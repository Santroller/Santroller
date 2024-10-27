
#define WII_TABLE(velocity) velocity << 2
#define WII_CROSSFADER(velocity) velocity << 4
#define WII_EFFECTSKNOB(velocity) velocity << 3
#define TRANSLATE_TURNTABLE_NO_FB(EFFECTS_KNOB, CROSSFADER, TABLE, SRC, DEST) \
    DEST->leftBlue |= SRC->leftBlue;                                          \
    DEST->leftRed |= SRC->leftRed;                                            \
    DEST->leftGreen |= SRC->leftGreen;                                        \
    DEST->rightBlue |= SRC->rightBlue;                                        \
    DEST->rightRed |= SRC->rightRed;                                          \
    DEST->rightGreen |= SRC->rightGreen;                                      \
    DEST->euphoria |= SRC->euphoria;                                          \
    if (SRC->effectsKnob) {                                                   \
        DEST->effectsKnob = EFFECTS_KNOB(SRC->effectsKnob);                   \
    }                                                                         \
    if (SRC->crossfader) {                                                    \
        DEST->crossfader = CROSSFADER(SRC->crossfader);                       \
    }                                                                         \
    if (SRC->leftTableVelocity) {                                             \
        DEST->leftTableVelocity = TABLE(SRC->leftTableVelocity);              \
    }                                                                         \
    if (SRC->rightTableVelocity) {                                            \
        DEST->rightTableVelocity = TABLE(SRC->rightTableVelocity);            \
    }
#define TRANSLATE_TURNTABLE(EFFECTS_KNOB, CROSSFADER, TABLE, SRC, DEST) \
    DEST->a |= SRC->a;                                                  \
    DEST->b |= SRC->b;                                                  \
    DEST->x |= SRC->x;                                                  \
    TRANSLATE_TURNTABLE_NO_FB(EFFECTS_KNOB, CROSSFADER, TABLE, SRC, DEST)
