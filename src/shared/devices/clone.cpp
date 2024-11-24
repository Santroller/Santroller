#include <stdint.h>

#include "Arduino.h"
#include "commands.h"
#include "config.h"
#include "defines.h"
#include "io.h"
#include "reports/controller_reports.h"
#include "state_translation/shared.h"
#include "state_translation/slider.h"
void clone_to_universal_report(USB_Host_Data_t *usb_host_data) {
    static uint8_t clone_data_read[4];
    static long clone_guitar_timer = 0;
    static long clone_guitar_ready_timer = 0;
    static bool clone_ready = false;
    static bool reading = false;
    static const uint8_t clone_data[] = {0x53, 0x10, 0x00, 0x01};
    if (!clone_guitar_ready_timer) {
        clone_guitar_ready_timer = millis();
    }
    if (millis() - clone_guitar_ready_timer > 350) {
        clone_ready = true;
    }
    if (clone_ready) {
        // The crazy guitar necks don't like being polled too quickly, this seems to work though.
        if (micros() - clone_guitar_timer > 4000) {
            if (!reading) {
                twi_writeTo(CLONE_TWI_PORT, CLONE_ADDR, clone_data, sizeof(clone_data), true, true);
                reading = true;
            } else {
                bool cloneValid = twi_readFrom(CLONE_TWI_PORT, CLONE_ADDR, clone_data_read, sizeof(clone_data_read), true);
                lastCloneWasSuccessful = cloneValid;
                if (!cloneValid) {
                    clone_ready = false;
                    clone_guitar_ready_timer = millis();
                }
                if (clone_data_read[0] == CLONE_VALID_PACKET) {
                    usb_host_data->soloGreen = clone_data_read[1] & 0x08;
                    usb_host_data->soloRed = clone_data_read[1] & 0x04;
                    usb_host_data->soloYellow = clone_data_read[1] & 0x02;
                    usb_host_data->soloBlue = clone_data_read[1] & 0x01;
                    usb_host_data->soloOrange = clone_data_read[2] & 0x80;
                    usb_host_data->green = clone_data_read[2] & 0x40;
                    usb_host_data->red = clone_data_read[2] & 0x01;
                    usb_host_data->yellow = clone_data_read[2] & 0x02;
                    usb_host_data->blue = clone_data_read[2] & 0x10;
                    usb_host_data->orange = clone_data_read[2] & 0x20;

                    solo_to_slider(usb_host_data);
                    reading = false;
                }
            }
            clone_guitar_timer = micros();
        }
    }
}