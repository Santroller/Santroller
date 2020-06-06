#pragma once
#include "controller/controller.h"
#include <stdint.h>
#include <LUFA/Common/Common.h>
typedef struct {
  uint8_t
      Button; /**< Button mask for currently pressed buttons in the mouse. */
  int8_t X;   /**< Current delta X movement of the mouse. */
  int8_t Y;   /**< Current delta Y movement on the mouse. */
  int8_t ScrollY; /** Current scroll Y delta movement on the mouse */
  int8_t ScrollX; /** Current scroll Y delta movement on the mouse */
} ATTR_PACKED USB_MouseReportScroll_Data_t;

void fillMouseReport(void *ReportData, uint16_t *const ReportSize,
                     Controller_t *controller);