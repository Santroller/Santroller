#include "mouse.h"
#include "config/eeprom.h"
#include "output/descriptors.h"
void fillMouseReport(void *ReportData, uint16_t *const ReportSize,
                     Controller_t *controller) {
  *ReportSize = sizeof(USB_MouseReportScroll_Data_t);
  USB_MouseReportScroll_Data_t *MouseReport =
      (USB_MouseReportScroll_Data_t *)ReportData;
  MouseReport->X = (controller->l_x >> 8) / 8;
  MouseReport->Y = (-(controller->l_y >> 8)) / 8;
  MouseReport->ScrollY = (controller->r_y >> 8) / 8;
  bit_write(bit_check(controller->buttons, XBOX_A), MouseReport->Button, 0);
  bit_write(bit_check(controller->buttons, XBOX_B), MouseReport->Button, 1);
  bit_write(bit_check(controller->buttons, XBOX_X), MouseReport->Button, 3);
}