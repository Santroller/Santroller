#include <stdint.h>

#include "defines.h"
#include "reports/controller_reports.h"
void gh5_to_universal_report(const uint8_t *data, uint8_t len, USB_Host_Data_t *usb_host_data);