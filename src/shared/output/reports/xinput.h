#pragma once
#include "controller/controller.h"
#include "output/controller_structs.h"
#include <stdint.h>
#include <string.h>

void fillXInputReport(void *ReportData, uint8_t *const ReportSize,
                      Controller_t *controller) {
  *ReportSize = sizeof(USB_XInputReport_Data_t);

  USB_XInputReport_Data_t *JoystickReport =
      (USB_XInputReport_Data_t *)ReportData;
  JoystickReport->rid = REPORT_ID_XINPUT;
  JoystickReport->rsize = sizeof(USB_XInputReport_Data_t);
  // Don't copy the led info tagged on the end
  memcpy(&JoystickReport->buttons, controller, sizeof(XInput_Data_t));
}