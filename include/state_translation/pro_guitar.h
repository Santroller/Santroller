#pragma once
typedef struct {
    uint16_t lowEFret : 5;
    uint16_t aFret : 5;
    uint16_t dFret : 5;
    uint16_t : 1; // always 1

    uint16_t gFret : 5;
    uint16_t bFret : 5;
    uint16_t highEFret : 5;
    uint16_t : 1; // always 0

    uint8_t : 1; // always 0
    uint8_t green :1;
    uint8_t red :1;
    uint8_t yellow :1;
    uint8_t blue :1;
    uint8_t orange :1;
    uint8_t soloFlag : 1;
    uint8_t : 1; // always 0
} __attribute__((packed)) protarneck_t;

#define TRANSLATE_PRO_GUITAR                                      \
    usb_host_data->lowEFretVelocity = report->lowEFretVelocity;   \
    usb_host_data->aFretVelocity = report->aFretVelocity;         \
    usb_host_data->dFretVelocity = report->dFretVelocity;         \
    usb_host_data->gFretVelocity = report->gFretVelocity;         \
    usb_host_data->bFretVelocity = report->bFretVelocity;         \
    usb_host_data->highEFretVelocity = report->highEFretVelocity; \
    usb_host_data->back |= report->back;                          \
    usb_host_data->start |= report->start;                        \
    usb_host_data->guide |= report->guide;                        \
                                                                  \
    usb_host_data->a |= report->a;                                \
    usb_host_data->b |= report->b;                                \
    usb_host_data->x |= report->x;                                \
    usb_host_data->y |= report->y;                                \
                                                                  \
    usb_host_data->green |= report->green;                        \
    usb_host_data->red |= report->red;                            \
    usb_host_data->yellow |= report->yellow;                      \
    usb_host_data->blue |= report->blue;                          \
    usb_host_data->orange |= report->orange;                      \
                                                                  \
    usb_host_data->lowEFret = report->lowEFret;                   \
    usb_host_data->aFret = report->aFret;                         \
    usb_host_data->dFret = report->dFret;                         \
    usb_host_data->gFret = report->gFret;                         \
    usb_host_data->bFret = report->bFret;                         \
    usb_host_data->highEFret = report->highEFret;                 \
                                                                  \
    usb_host_data->tilt = report->tilt == 0x7F ? INT16_MAX : 0;   \
    /* TODO: pedalDigital should map to pedalDigital*/            \
    usb_host_data->back |= report->pedal;

#define TRANSLATE_TO_PRO_GUITAR(item)                                  \
    report->lowEFretVelocity = item.lowEFretVelocity;                  \
    report->aFretVelocity = item.aFretVelocity;                        \
    report->dFretVelocity = item.dFretVelocity;                        \
    report->gFretVelocity = item.gFretVelocity;                        \
    report->bFretVelocity = item.bFretVelocity;                        \
    report->highEFretVelocity = item.highEFretVelocity;                \
    report->tilt = item.tilt == INT16_MAX ? 0x7F : 0x60;               \
    report->green = item.green;                                        \
    report->red = item.red;                                            \
    report->yellow = item.yellow;                                      \
    report->blue = item.blue;                                          \
    report->orange = item.orange;                                      \
    report->lowEFret = item.lowEFret;                                  \
    report->aFret = item.aFret;                                        \
    report->dFret = item.dFret;                                        \
    report->gFret = item.gFret;                                        \
    report->bFret = item.bFret;                                        \
    report->highEFret = item.highEFret;
