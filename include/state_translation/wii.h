#include <stdint.h>
#include <string.h>

#include "defines.h"
#include "reports/controller_reports.h"
#include "state_translation/shared.h"
#include "state_translation/slider.h"

typedef struct {
    union {
        signed int ltt : 6;
        struct {
            unsigned int ltt40 : 5;
            unsigned int ltt5 : 1;
        };
    };
} ltt_t;
typedef struct {
    union {
        signed int rtt : 6;
        struct {
            unsigned int rtt0 : 1;
            unsigned int rtt21 : 2;
            unsigned int rtt43 : 2;
            unsigned int rtt5 : 1;
        };
    };
} rtt_t;
enum DrumType {
    DRUM_GREEN,
    DRUM_RED,
    DRUM_YELLOW,
    DRUM_BLUE,
    DRUM_ORANGE,
    DRUM_KICK,
    DRUM_HIHAT
};
extern uint8_t drumVelocity[8];
extern bool hasTapBar;
void wii_to_universal_report(const uint8_t *data, uint8_t len, uint16_t controllerType, bool hiRes, USB_Host_Data_t *usb_host_data);
uint8_t universal_report_to_wii(uint8_t *data, uint8_t sub_type, uint8_t format, const USB_Host_Data_t *usb_host_data);