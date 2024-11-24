#include <stdint.h>
#include <string.h>

#include "defines.h"
#include "reports/controller_reports.h"
#include "state_translation/shared.h"
uint8_t universal_report_to_festival_hid(uint8_t *data, uint8_t sub_type, const USB_Host_Data_t *usb_host_data);