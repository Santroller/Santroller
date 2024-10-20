
// Translate a Pro keys style report to a usb host report
// Note that we temporarily clear the buttons and then merge after
// As that makes it easier to process the velocities
#define TRANSLATE_PRO_KEYS                                                             \
    usb_host_data->a |= report->a;                                                     \
    usb_host_data->b |= report->b;                                                     \
    usb_host_data->x |= report->x;                                                     \
    usb_host_data->y |= report->y;                                                     \
    usb_host_data->back |= report->back;                                               \
    usb_host_data->start |= report->start;                                             \
    usb_host_data->guide |= report->guide;                                             \
    uint32_t old_buttons = usb_host_data->proKeyButtons;                               \
    usb_host_data->proKeyButtons = 0;                                                  \
    usb_host_data->key1 |= report->key1;                                               \
    usb_host_data->key2 |= report->key2;                                               \
    usb_host_data->key3 |= report->key3;                                               \
    usb_host_data->key4 |= report->key4;                                               \
    usb_host_data->key5 |= report->key5;                                               \
    usb_host_data->key6 |= report->key6;                                               \
    usb_host_data->key7 |= report->key7;                                               \
    usb_host_data->key8 |= report->key8;                                               \
                                                                                       \
    usb_host_data->key9 |= report->key9;                                               \
    usb_host_data->key10 |= report->key10;                                             \
    usb_host_data->key11 |= report->key11;                                             \
    usb_host_data->key12 |= report->key12;                                             \
    usb_host_data->key13 |= report->key13;                                             \
    usb_host_data->key14 |= report->key14;                                             \
    usb_host_data->key15 |= report->key15;                                             \
    usb_host_data->key16 |= report->key16;                                             \
                                                                                       \
    usb_host_data->key17 |= report->key17;                                             \
    usb_host_data->key18 |= report->key18;                                             \
    usb_host_data->key19 |= report->key19;                                             \
    usb_host_data->key20 |= report->key20;                                             \
    usb_host_data->key21 |= report->key21;                                             \
    usb_host_data->key22 |= report->key22;                                             \
    usb_host_data->key23 |= report->key23;                                             \
    usb_host_data->key24 |= report->key24;                                             \
    usb_host_data->key25 |= report->key25;                                             \
    usb_host_data->overdrive |= report->overdrive;                                     \
    usb_host_data->pedalDigital |= report->pedalDigital;                               \
    if (report->pedalAnalog) {                                                         \
        usb_host_data->pedalAnalog = report->pedalAnalog << 1;                         \
    }                                                                                  \
    if (report->touchPad) {                                                            \
        usb_host_data->touchPad = report->touchPad << 1;                               \
    }                                                                                  \
                                                                                       \
    uint8_t currentVel = 0;                                                            \
    for (int i = 0; i < 25; i++) {                                                     \
        if (usb_host_data->proKeyButtons & i) {                                        \
            if (currentVel < 5) {                                                      \
                usb_host_data->proKeyVelocities[i] = report->velocities[currentVel++]; \
            } else {                                                                   \
                usb_host_data->proKeyVelocities[i] = 64;                               \
            }                                                                          \
        }                                                                              \
    }                                                                                  \
    usb_host_data->proKeyButtons |= old_buttons;
