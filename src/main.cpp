#include <pb_decode.h>
#include <pb_encode.h>
#include <stdint.h>
#include <stdio.h>
#include <map>

#include "config.pb.h"
#include "pico/stdlib.h"
#include "FlashPROM.h"
#include "CRC32.h"
#include "pico/multicore.h"
#include "config.hpp"

#include "tusb.h"

#include "usb_descriptors.h"

void core1()
{
}
void hid_task(void)
{
}
int main()
{
    multicore_launch_core1(core1);
    stdio_init_all();
    EEPROM.start();

    proto_Config config;
    load(config);

    // init device stack on configured roothub port
    tud_init(BOARD_TUD_RHPORT);

    while (1)
    {
        tud_task(); // tinyusb device task

        hid_task();
    }
    return 0;
}

// Invoked when sent REPORT successfully to host
// Application can use this to send the next report
// Note: For composite reports, report[0] is report ID
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint16_t len)
{
  (void) instance;
  (void) len;
}
uint32_t start = 0;
// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
  // TODO not Implemented
  (void) instance;
  (void) report_id;
  (void) report_type;
  (void) buffer;
  (void) reqlen;
  if (report_type == HID_REPORT_TYPE_FEATURE && report_id == REPORT_ID_CONFIG) {
    uint32_t ret = copy_config(buffer, start);
    start += ret;
    return ret;
  }
  if (report_type == HID_REPORT_TYPE_FEATURE && report_id == REPORT_ID_CONFIG_INFO) {
    start = 0;
    return copy_config_info(buffer);
  }

  return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
  (void) instance;

}