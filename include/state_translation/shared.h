#include <stdint.h>
extern const uint8_t dpad_bindings[11];
extern const uint8_t dpad_bindings_reverse[8];
#define DPAD_REV()                                                                 \
    uint8_t dpad = report->dpad >= 0x08 ? 0 : dpad_bindings_reverse[report->dpad]; \
    asm volatile("" ::                                                             \
                     : "memory");                                                  \
    usb_host_data->dpadLeft |= dpad & LEFT;                                        \
    usb_host_data->dpadRight |= dpad & RIGHT;                                      \
    usb_host_data->dpadUp |= dpad & UP;                                            \
    usb_host_data->dpadDown |= dpad & DOWN;
