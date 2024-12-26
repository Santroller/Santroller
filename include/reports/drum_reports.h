#include <stdint.h>
#define YELLOW 0
#define RED 1
#define GREEN 2
#define BLUE 3
#define CYMBAL 4
typedef union {
    struct {
        uint8_t yellow : 1;
        uint8_t red : 1;
        uint8_t green : 1;
        uint8_t blue : 1;

        uint8_t yellowCymbal : 1;
        uint8_t : 1;
        uint8_t greenCymbal : 1;
        uint8_t blueCymbal : 1;
    };
    uint8_t buttons;
} USB_RB_Drums_t;
#define SET_PADS()                                                              \
    bool green = report->a;                                                     \
    bool red = report->b;                                                       \
    bool yellow = report->y;                                                    \
    bool blue = report->x;                                                      \
    bool pad = report->padFlag;                                                 \
    bool cymbal = report->cymbalFlag;                                           \
    if (pad || cymbal) {                                                        \
        hasFlags = true;                                                        \
    }                                                                           \
    if (pad && cymbal) {                                                        \
        int colorCount = 0;                                                     \
        colorCount += red ? 1 : 0;                                              \
        colorCount += (yellow || report->dpadUp) ? 1 : 0;                       \
        colorCount += (blue || report->dpadDown) ? 1 : 0;                       \
        colorCount += (green || !(report->dpadUp || report->dpadDown)) ? 1 : 0; \
                                                                                \
        if (colorCount > 1) {                                                   \
            if (report->dpadUp) {                                               \
                usb_host_data->yellowCymbal = true;                             \
                yellow = false;                                                 \
                cymbal = false;                                                 \
            }                                                                   \
            if (report->dpadDown) {                                             \
                usb_host_data->blueCymbal = true;                               \
                blue = false;                                                   \
                cymbal = false;                                                 \
            }                                                                   \
            if (!(report->dpadUp || report->dpadDown)) {                        \
                usb_host_data->greenCymbal = true;                              \
                green = false;                                                  \
                cymbal = false;                                                 \
            }                                                                   \
        }                                                                       \
    }                                                                           \
    if (pad || (!cymbal && !hasFlags)) {                                        \
        usb_host_data->red |= red;                                              \
        usb_host_data->green |= green;                                          \
        usb_host_data->yellow |= yellow;                                        \
        usb_host_data->blue |= blue;                                            \
    }                                                                           \
                                                                                \
    if (cymbal) {                                                               \
        usb_host_data->greenCymbal |= green;                                    \
        usb_host_data->blueCymbal |= blue;                                      \
        usb_host_data->yellowCymbal |= yellow;                                  \
    }
