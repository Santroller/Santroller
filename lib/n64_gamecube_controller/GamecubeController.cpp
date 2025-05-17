#include "GamecubeController.h"

#include "Controller.pio.h"
#include "pico/stdlib.h"

void GamecubeController::init() {
  uint8_t request[1] = {0x41};
  uint8_t response[3];
  transfer(request, sizeof(request), response, sizeof(response));
}

void GamecubeController::updateState() {
  uint8_t setRumble = _rumble ? 1 : 0;
  uint8_t request[3] = {0x40, 0x03, setRumble};
  transfer(request, sizeof(request), _controllerState, _sizeofControllerState);
}

// void GamecubeController::getSwitchReport(SwitchReport *switchReport) {
//   updateState();

//   switchReport->buttons[0] =
//       (!(GC_MASK_R & _controllerState[1]) && _controllerState[7] > 0x30
//            ? SWITCH_MASK_ZR
//            : 0) |
//       (GC_MASK_R & _controllerState[1] ? SWITCH_MASK_R : 0) |
//       (GC_MASK_A & _controllerState[0] ? SWITCH_MASK_A : 0) |
//       (GC_MASK_B & _controllerState[0] ? SWITCH_MASK_B : 0) |
//       (GC_MASK_X & _controllerState[0] ? SWITCH_MASK_X : 0) |
//       (GC_MASK_Y & _controllerState[0] ? SWITCH_MASK_Y : 0);

//   switchReport->buttons[1] =
//       ((GC_MASK_L & _controllerState[1]) && (GC_MASK_R & _controllerState[1]) &&
//                (GC_MASK_START & _controllerState[0])
//            ? SWITCH_MASK_HOME
//            : 0) |
//       (GC_MASK_START & _controllerState[0] ? SWITCH_MASK_PLUS : 0);

//   switchReport->buttons[2] = 0x00;
//   switch (GC_MASK_DPAD & _controllerState[1]) {
//     case GC_MASK_DPAD_UP:
//       switchReport->buttons[2] |= SWITCH_HAT_UP;
//       break;
//     case GC_MASK_DPAD_UPRIGHT:
//       switchReport->buttons[2] |= SWITCH_HAT_UPRIGHT;
//       break;
//     case GC_MASK_DPAD_RIGHT:
//       switchReport->buttons[2] |= SWITCH_HAT_RIGHT;
//       break;
//     case GC_MASK_DPAD_DOWNRIGHT:
//       switchReport->buttons[2] |= SWITCH_HAT_DOWNRIGHT;
//       break;
//     case GC_MASK_DPAD_DOWN:
//       switchReport->buttons[2] |= SWITCH_HAT_DOWN;
//       break;
//     case GC_MASK_DPAD_DOWNLEFT:
//       switchReport->buttons[2] |= SWITCH_HAT_DOWNLEFT;
//       break;
//     case GC_MASK_DPAD_LEFT:
//       switchReport->buttons[2] |= SWITCH_HAT_LEFT;
//       break;
//     case GC_MASK_DPAD_UPLEFT:
//       switchReport->buttons[2] |= SWITCH_HAT_UPLEFT;
//       break;
//   }

//   switchReport->buttons[2] |=
//       (GC_MASK_L & _controllerState[1] ? SWITCH_MASK_L : 0) |
//       (!(GC_MASK_L & _controllerState[1]) && _controllerState[6] > 0x30
//            ? SWITCH_MASK_ZL
//            : 0) |
//       (GC_MASK_Z & _controllerState[1] ? SWITCH_MASK_L : 0);

//   // Scale for joystick insensitivity if needed
//   uint16_t lx =
//       convertToSwitchJoystick(_controllerState[2], &_minAnalogX, &_maxAnalogX);
//   uint16_t ly =
//       convertToSwitchJoystick(_controllerState[3], &_minAnalogY, &_maxAnalogY);
//   switchReport->l[0] = lx & 0xff;
//   switchReport->l[1] = ((ly & 0xf) << 4) | (lx >> 8);
//   switchReport->l[2] = ly >> 4;

//   uint16_t rx = convertToSwitchJoystick(_controllerState[4], &_minCX, &_maxCX);
//   uint16_t ry = convertToSwitchJoystick(_controllerState[5], &_minCY, &_maxCY);
//   switchReport->r[0] = rx & 0xff;
//   switchReport->r[1] = ((ry & 0xf) << 4) | (rx >> 8);
//   switchReport->r[2] = ry >> 4;

//   return;
// }

// uint16_t GamecubeController::convertToSwitchJoystick(uint8_t axisPos,
//                                                      double *minAxis,
//                                                      double *maxAxis) {
//   double unscaledAxisPos =
//       (axisPos - GC_JOYSTICK_MID) / (double)GC_JOYSTICK_MID;
//   double scaledAxisPos = getScaledAnalogAxis(unscaledAxisPos, minAxis, maxAxis);
//   return scaledAxisPos * SWITCH_JOYSTICK_MID + SWITCH_JOYSTICK_MID + 1;
// }