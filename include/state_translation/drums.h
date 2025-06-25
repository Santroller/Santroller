#include <stdint.h>
#define YELLOW 0
#define RED 1
#define GREEN 2
#define BLUE 3
#define CYMBAL 4

#define SET_RB_PADS()                                                                                             \
    bool green = report->a;                                                                                       \
    bool red = report->b;                                                                                         \
    bool yellow = report->y;                                                                                      \
    bool blue = report->x;                                                                                        \
    bool pad = report->padFlag;                                                                                   \
    bool cymbal = report->cymbalFlag;                                                                             \
    if (!red) {                                                                                                   \
        offNote(10, RB_MIDI_NOTE_RED, 0);                                                                         \
    }                                                                                                             \
    if (!green) {                                                                                                 \
        offNote(10, RB_MIDI_NOTE_GREEN, 0);                                                                       \
        offNote(10, RB_MIDI_NOTE_GREEN_CYMBAL, 0);                                                                \
    }                                                                                                             \
    if (!yellow) {                                                                                                \
        offNote(10, RB_MIDI_NOTE_YELLOW, 0);                                                                      \
        offNote(10, RB_MIDI_NOTE_YELLOW_CYMBAL, 0);                                                               \
    }                                                                                                             \
    if (!blue) {                                                                                                  \
        offNote(10, RB_MIDI_NOTE_BLUE, 0);                                                                        \
        offNote(10, RB_MIDI_NOTE_BLUE_CYMBAL, 0);                                                                 \
    }                                                                                                             \
    if (pad && cymbal) {                                                                                          \
        /* The d-pad inputs let us resolve the ambiguity of a pad+cymbal hit                                      \
           Only d-pad is checked here since it is the only unique identifier due to hardware bugs                 \
           (sometimes the color flag is released before the d-pad input is, particularly at high polling rates)*/ \
                                                                                                                  \
        /* If you hit both the pad and cymbal of the same colour simultaneously, the velocity for the cymbal      \
            is placed in the redVelocity, but the red button won't be pressed. */                                 \
        if (!red && redVelocity) {                                                                                \
            if (up) {                                                                                             \
                onNote(10, RB_MIDI_NOTE_YELLOW_CYMBAL, redVelocity >> 1);                                         \
                onNote(10, RB_MIDI_NOTE_YELLOW, yellowVelocity >> 1);                                             \
            } else if (down) {                                                                                    \
                onNote(10, RB_MIDI_NOTE_BLUE_CYMBAL, redVelocity >> 1);                                           \
                onNote(10, RB_MIDI_NOTE_BLUE, blueVelocity >> 1);                                                 \
            } else {                                                                                              \
                onNote(10, RB_MIDI_NOTE_GREEN_CYMBAL, redVelocity >> 1);                                          \
                onNote(10, RB_MIDI_NOTE_GREEN, greenVelocity >> 1);                                               \
            }                                                                                                     \
            pad = false;                                                                                          \
            cymbal = false;                                                                                       \
        } else {                                                                                                  \
            /* Yellow */                                                                                          \
            if (up) {                                                                                             \
                onNote(10, RB_MIDI_NOTE_YELLOW_CYMBAL, yellowVelocity >> 1);                                      \
                yellow = false;                                                                                   \
                cymbal = false;                                                                                   \
            }                                                                                                     \
                                                                                                                  \
            /* Blue */                                                                                            \
            if (down) {                                                                                           \
                onNote(10, RB_MIDI_NOTE_BLUE_CYMBAL, blueVelocity >> 1);                                          \
                blue = false;                                                                                     \
                cymbal = false;                                                                                   \
            }                                                                                                     \
                                                                                                                  \
            /* Green */                                                                                           \
            if (!(up || down)) {                                                                                  \
                onNote(10, RB_MIDI_NOTE_GREEN_CYMBAL, blueVelocity >> 1);                                         \
                green = false;                                                                                    \
                cymbal = false;                                                                                   \
            }                                                                                                     \
        }                                                                                                         \
    }                                                                                                             \
    if (pad) {                                                                                                    \
        if (yellow) {                                                                                             \
            onNote(10, RB_MIDI_NOTE_YELLOW, yellowVelocity >> 1);                                                 \
        }                                                                                                         \
        if (blue) {                                                                                               \
            onNote(10, RB_MIDI_NOTE_BLUE, blueVelocity >> 1);                                                     \
        }                                                                                                         \
        if (red) {                                                                                                \
            onNote(10, RB_MIDI_NOTE_RED, redVelocity >> 1);                                                       \
        }                                                                                                         \
        if (green) {                                                                                              \
            onNote(10, RB_MIDI_NOTE_GREEN, greenVelocity >> 1);                                                   \
        }                                                                                                         \
    }                                                                                                             \
                                                                                                                  \
    if (cymbal) {                                                                                                 \
        if (blue) {                                                                                               \
            onNote(10, RB_MIDI_NOTE_BLUE_CYMBAL, blueVelocity >> 1);                                              \
        }                                                                                                         \
        if (red) {                                                                                                \
            onNote(10, RB_MIDI_NOTE_YELLOW_CYMBAL, redVelocity >> 1);                                             \
        }                                                                                                         \
        if (green) {                                                                                              \
            onNote(10, RB_MIDI_NOTE_GREEN_CYMBAL, greenVelocity >> 1);                                            \
        }                                                                                                         \
    }                                                                                                             \
    if (kick1) {                                                                                                  \
        onNote(10, RB_MIDI_NOTE_KICK, 0x7F);                                                                      \
    } else {                                                                                                      \
        offNote(10, RB_MIDI_NOTE_KICK, 0);                                                                        \
    }                                                                                                             \
    if (kick2) {                                                                                                  \
        onNote(10, RB_MIDI_NOTE_KICK2, 0x7F);                                                                     \
    } else {                                                                                                      \
        offNote(10, RB_MIDI_NOTE_KICK2, 0);                                                                       \
    }
#if DEVICE_TYPE == ROCK_BAND_DRUMS
#define SET_GH_PADS()                                             \
    if (report->greenVelocity) {                                  \
        onNote(10, RB_MIDI_NOTE_GREEN, report->greenVelocity);    \
    } else {                                                      \
        offNote(10, RB_MIDI_NOTE_GREEN, report->greenVelocity);   \
    }                                                             \
    if (report->redVelocity) {                                    \
        onNote(10, RB_MIDI_NOTE_RED, report->redVelocity);        \
    } else {                                                      \
        offNote(10, RB_MIDI_NOTE_RED, report->redVelocity);       \
    }                                                             \
    if (report->yellowVelocity) {                                 \
        onNote(10, RB_MIDI_NOTE_YELLOW, report->yellowVelocity);  \
    } else {                                                      \
        offNote(10, RB_MIDI_NOTE_YELLOW, report->yellowVelocity); \
    }                                                             \
    if (report->blueVelocity) {                                   \
        onNote(10, RB_MIDI_NOTE_BLUE, report->blueVelocity);      \
    } else {                                                      \
        offNote(10, RB_MIDI_NOTE_BLUE, report->blueVelocity);     \
    }                                                             \
    if (report->orangeVelocity) {                                 \
        onNote(10, RB_MIDI_NOTE_GREEN, report->orangeVelocity);  \
    } else {                                                      \
        offNote(10, RB_MIDI_NOTE_GREEN, report->orangeVelocity); \
    }                                                             \
    if (report->kickVelocity) {                                   \
        onNote(10, RB_MIDI_NOTE_KICK, report->kickVelocity);      \
    } else {                                                      \
        offNote(10, RB_MIDI_NOTE_KICK, report->kickVelocity);     \
    }
#else
#define SET_GH_PADS()                                             \
    if (report->greenVelocity) {                                  \
        onNote(10, GH_REM_MIDI_NOTE_GREEN, report->greenVelocity);    \
    } else {                                                      \
        offNote(10, GH_REM_MIDI_NOTE_GREEN, report->greenVelocity);   \
    }                                                             \
    if (report->redVelocity) {                                    \
        onNote(10, GH_REM_MIDI_NOTE_RED, report->redVelocity);        \
    } else {                                                      \
        offNote(10, GH_REM_MIDI_NOTE_RED, report->redVelocity);       \
    }                                                             \
    if (report->yellowVelocity) {                                 \
        onNote(10, GH_REM_MIDI_NOTE_YELLOW, report->yellowVelocity);  \
    } else {                                                      \
        offNote(10, GH_REM_MIDI_NOTE_YELLOW, report->yellowVelocity); \
    }                                                             \
    if (report->blueVelocity) {                                   \
        onNote(10, GH_REM_MIDI_NOTE_BLUE, report->blueVelocity);      \
    } else {                                                      \
        offNote(10, GH_REM_MIDI_NOTE_BLUE, report->blueVelocity);     \
    }                                                             \
    if (report->orangeVelocity) {                                 \
        onNote(10, GH_REM_MIDI_NOTE_ORANGE, report->orangeVelocity);  \
    } else {                                                      \
        offNote(10, GH_REM_MIDI_NOTE_ORANGE, report->orangeVelocity); \
    }                                                             \
    if (report->kickVelocity) {                                   \
        onNote(10, GH_REM_MIDI_NOTE_KICK, report->kickVelocity);      \
    } else {                                                      \
        offNote(10, GH_REM_MIDI_NOTE_KICK, report->kickVelocity);     \
    }
#endif