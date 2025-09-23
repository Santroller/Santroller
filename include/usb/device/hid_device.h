
#pragma once
#include "usb/usb_descriptors.h"
#include "console_mode.h"

#include "tusb.h"

uint16_t tud_hid_generic_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen);
void tud_hid_generic_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize);
bool tud_hid_generic_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request);