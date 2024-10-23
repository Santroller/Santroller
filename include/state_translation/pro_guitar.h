#define TRANSLATE_PRO_GUITAR                                      \
    usb_host_data->back |= report->back;                          \
    usb_host_data->start |= report->start;                        \
    usb_host_data->guide |= report->guide;                        \
                                                                  \
    usb_host_data->a |= report->a;                                \
    usb_host_data->b |= report->b;                                \
    usb_host_data->x |= report->x;                                \
    usb_host_data->y |= report->y;                                \
                                                                  \
    usb_host_data->green |= report->greenFret;                    \
    usb_host_data->red |= report->redFret;                        \
    usb_host_data->yellow |= report->yellowFret;                  \
    usb_host_data->blue |= report->blueFret;                      \
    usb_host_data->orange |= report->orangeFret;                  \
                                                                  \
    usb_host_data->lowEFret = report->lowEFret;                   \
    usb_host_data->aFret = report->aFret;                         \
    usb_host_data->dFret = report->dFret;                         \
    usb_host_data->gFret = report->gFret;                         \
    usb_host_data->bFret = report->bFret;                         \
    usb_host_data->highEFret = report->highEFret;                 \
                                                                  \
    usb_host_data->lowEFretVelocity = report->lowEFretVelocity;   \
    usb_host_data->aFretVelocity = report->aFretVelocity;         \
    usb_host_data->dFretVelocity = report->dFretVelocity;         \
    usb_host_data->gFretVelocity = report->gFretVelocity;         \
    usb_host_data->bFretVelocity = report->bFretVelocity;         \
    usb_host_data->highEFretVelocity = report->highEFretVelocity; \
                                                                  \
    usb_host_data->tilt |= report->tilt;                          \
    usb_host_data->pedalDigital |= report->pedal;

#define TRANSLATE_TO_PRO_GUITAR(item)                   \
    report->lowEFret = item.lowEFret;                   \
    report->aFret = item.aFret;                         \
    report->dFret = item.dFret;                         \
    report->gFret = item.gFret;                         \
    report->bFret = item.bFret;                         \
    report->highEFret = item.highEFret;

#define TRANSLATE_TO_PRO_GUITAR_BOTH(item)              \
    report->lowEFretVelocity = item.lowEFretVelocity;   \
    report->aFretVelocity = item.aFretVelocity;         \
    report->dFretVelocity = item.dFretVelocity;         \
    report->gFretVelocity = item.gFretVelocity;         \
    report->bFretVelocity = item.bFretVelocity;         \
    report->highEFretVelocity = item.highEFretVelocity; \
    TRANSLATE_TO_PRO_GUITAR(item)
