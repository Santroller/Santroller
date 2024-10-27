#define TRANSLATE_PRO_GUITAR(SRC, DEST)               \
    DEST->lowEFretVelocity = SRC->lowEFretVelocity;   \
    DEST->aFretVelocity = SRC->aFretVelocity;         \
    DEST->dFretVelocity = SRC->dFretVelocity;         \
    DEST->gFretVelocity = SRC->gFretVelocity;         \
    DEST->bFretVelocity = SRC->bFretVelocity;         \
    DEST->highEFretVelocity = SRC->highEFretVelocity; \
                                                      \
    DEST->a |= SRC->a;                                \
    DEST->b |= SRC->b;                                \
    DEST->x |= SRC->x;                                \
    DEST->y |= SRC->y;                                \
                                                      \
    DEST->green |= SRC->green;                        \
    DEST->red |= SRC->red;                            \
    DEST->yellow |= SRC->yellow;                      \
    DEST->blue |= SRC->blue;                          \
    DEST->orange |= SRC->orange;                      \
                                                      \
    DEST->lowEFret = SRC->lowEFret;                   \
    DEST->aFret = SRC->aFret;                         \
    DEST->dFret = SRC->dFret;                         \
    DEST->gFret = SRC->gFret;                         \
    DEST->bFret = SRC->bFret;                         \
    DEST->highEFret = SRC->highEFret;                 \
                                                      \
    DEST->tilt |= SRC->tilt;                          \
    DEST->pedal |= SRC->pedal;
    