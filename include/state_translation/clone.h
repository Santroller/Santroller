#include <stdint.h>

#include "Arduino.h"
#include "commands.h"
#include "config.h"
#include "defines.h"
#include "io.h"
#include "reports/controller_reports.h"
#include "state_translation/shared.h"
#include "state_translation/slider.h"
void clone_to_universal_report(USB_Host_Data_t *usb_host_data);