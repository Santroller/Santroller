
#pragma once
#include "usb/usb_descriptors.h"

#include "tusb.h"

#define PS4_LED_RUMBLE_ID 0x05
#define PS4_GAMEPAD 0
#define PS4_GUITAR 1
#define PS4_DRUMS 2
#define PS4_FIGHTSTICK 7  // Useful on PS5?
uint16_t tud_hid_ps4_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen);
void tud_hid_ps4_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize);
void tud_set_lightbar_led_cb(uint8_t red, uint8_t green, uint8_t blue);