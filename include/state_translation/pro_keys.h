
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
    prokey_buttons_t buttons = {0};                                                    \
    buttons.proKeyButtons = 0;                                                        \
    buttons.key1 = report->key1;                                                     \
    buttons.key2 = report->key2;                                                     \
    buttons.key3 = report->key3;                                                     \
    buttons.key4 = report->key4;                                                     \
    buttons.key5 = report->key5;                                                     \
    buttons.key6 = report->key6;                                                     \
    buttons.key7 = report->key7;                                                     \
    buttons.key8 = report->key8;                                                     \
                                                                                       \
    buttons.key9 = report->key9;                                                     \
    buttons.key10 = report->key10;                                                   \
    buttons.key11 = report->key11;                                                   \
    buttons.key12 = report->key12;                                                   \
    buttons.key13 = report->key13;                                                   \
    buttons.key14 = report->key14;                                                   \
    buttons.key15 = report->key15;                                                   \
    buttons.key16 = report->key16;                                                   \
                                                                                       \
    buttons.key17 = report->key17;                                                   \
    buttons.key18 = report->key18;                                                   \
    buttons.key19 = report->key19;                                                   \
    buttons.key20 = report->key20;                                                   \
    buttons.key21 = report->key21;                                                   \
    buttons.key22 = report->key22;                                                   \
    buttons.key23 = report->key23;                                                   \
    buttons.key24 = report->key24;                                                   \
    buttons.key25 = report->key25;                                                   \
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
        if (buttons.proKeyButtons & i) {                                        \
            if (currentVel < 5) {                                                      \
                usb_host_data->proKeyVelocities[i] = report->velocities[currentVel++]; \
            } else {                                                                   \
                usb_host_data->proKeyVelocities[i] = 64;                               \
            }                                                                          \
        }                                                                              \
    }                                                                                  
