#pragma once
#include "eeprom/eeprom.h"
#include "controller/controller.h"
#include "output/controller_structs.h"
#include "output/descriptors.h"
#include <LUFA/Common/Common.h>
#include <stdint.h>
void fillMouseReport(void *ReportData, uint8_t *const ReportSize,
                     Controller_t *controller) {
  *ReportSize = sizeof(USB_ID_MouseReport_Data_t);
  USB_ID_MouseReport_Data_t *MouseReport =
      (USB_ID_MouseReport_Data_t *)ReportData;
  MouseReport->rid = REPORT_ID_MOUSE;
  MouseReport->X = (controller->l_x >> 8) / 8;
  MouseReport->Y = (-(controller->l_y >> 8)) / 8;
  MouseReport->ScrollY = (controller->r_y >> 8) / 8;
  MouseReport->ScrollX = (controller->r_x >> 8) / 8;
  bit_write(bit_check(controller->buttons, XBOX_A), MouseReport->Button, 0);
  bit_write(bit_check(controller->buttons, XBOX_B), MouseReport->Button, 1);
  bit_write(bit_check(controller->buttons, XBOX_X), MouseReport->Button, 3);
}