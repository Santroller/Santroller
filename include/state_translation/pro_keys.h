#define PRO_KEY_COUNT 25
#define PRO_KEY_FIRST_NOTE 24
#define DEFAULT_VELOCITY 64
// Translate a Pro keys style report to a usb host report
// Note that we temporarily clear the buttons and then merge after
// As that makes it easier to process the velocities
#define TRANSLATE_PRO_KEYS                                                                 \
    usb_host_data->a |= report->a;                                                         \
    usb_host_data->b |= report->b;                                                         \
    usb_host_data->x |= report->x;                                                         \
    usb_host_data->y |= report->y;                                                         \
    usb_host_data->back |= report->back;                                                   \
    usb_host_data->start |= report->start;                                                 \
    usb_host_data->guide |= report->guide;                                                 \
    if (report->pedalDigital) {                                                            \
        onControlChange(0, MIDI_CONTROL_COMMAND_SUSTAIN_PEDAL, 0x7F);                      \
    } else if (~(report->pedalAnalog)) {                                                   \
        onControlChange(0, MIDI_CONTROL_COMMAND_SUSTAIN_PEDAL, ~(report->pedalAnalog));    \
    } else {                                                                               \
        onControlChange(0, MIDI_CONTROL_COMMAND_SUSTAIN_PEDAL, 0);                         \
    }                                                                                      \
    onControlChange(0, MIDI_CONTROL_COMMAND_MOD_WHEEL, report->overdrive ? 0xFF : 0x00);   \
    /* uint8 -> int14 (but only the positive half!) */                                     \
    onPitchBend(0, report->touchPad << 5);                                                 \
    uint32_t keyMask =                                                                     \
        (report->keys[0] << 17) |                                                          \
        (report->keys[1] << 9) |                                                           \
        (report->keys[0] << 1) |                                                           \
        ((report->velocities[0] & 0x80) >> 7);                                             \
                                                                                           \
    int pressed = 0;                       /* Number of keys pressed */                    \
    int keyBit = 1 << (PRO_KEY_COUNT - 1); /* Bitmask used to test for a specific key */   \
    for (int i = 0; i < PRO_KEY_COUNT; i++) {                                              \
        /* Check if the key is pressed */                                                  \
        if (keyMask & keyBit) {                                                            \
            /* There are only 5 key velocities stored*/                                    \
            if (pressed < 5) {                                                             \
                /* Retrieve velocity (ranges from 0-127)*/                                 \
                onNote(1, PRO_KEY_FIRST_NOTE + i, (report->velocities[pressed++] & 0x7F)); \
            } else {                                                                       \
                onNote(1, PRO_KEY_FIRST_NOTE + i, DEFAULT_VELOCITY);                       \
            }                                                                              \
        } else {                                                                           \
            offNote(1, PRO_KEY_FIRST_NOTE + i, 0);                                         \
        }                                                                                  \
                                                                                           \
        /* Shift the key mask to the next key*/                                            \
        keyMask >>= 1;                                                                     \
    }
