#include <stdint.h>

#include "defines.h"
#include "config.h"
#include "reports/controller_reports.h"
#include "state_translation/shared.h"
extern uint8_t gh5_mapping[32];
void solo_to_slider(USB_Host_Data_t *usb_host_data) {
    usb_host_data->slider = gh5_mapping[usb_host_data->soloGreen | usb_host_data->soloRed << 1 | usb_host_data->soloYellow << 2 | usb_host_data->soloBlue << 3 | usb_host_data->soloOrange << 4];
}

void slider_to_solo(USB_Host_Data_t *usb_host_data) {
    for (uint8_t i = 0; i < sizeof(gh5_mapping); i++) {
        if (gh5_mapping[i] == usb_host_data->slider) {
            usb_host_data->soloGreen = i & 1;
            usb_host_data->soloRed = i & 1 << 1;
            usb_host_data->soloYellow = i & 1 << 2;
            usb_host_data->soloBlue = i & 1 << 3;
            usb_host_data->soloOrange = i & 1 << 4;
        }
    }
}