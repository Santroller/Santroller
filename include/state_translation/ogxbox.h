#include <stdint.h>

#include "defines.h"
#include "reports/controller_reports.h"
#include "state_translation/shared.h"
void ogxbox_to_universal_report(const uint8_t *data, uint8_t len, uint8_t sub_type, USB_Host_Data_t *usb_host_data);
uint8_t universal_report_to_ogxbox(uint8_t *data, uint8_t sub_type, const USB_Host_Data_t *usb_host_data);