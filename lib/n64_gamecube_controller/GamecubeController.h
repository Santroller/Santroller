#ifndef GamecubeController_h
#define GamecubeController_h
#include <string.h>

#include "Controller.h"

class GamecubeController : public Controller {
 public:
  GamecubeController(InitParams *initParams) : Controller(initParams, 8){};
  void init();
  // void getSwitchReport(SwitchReport *switchReport);
  void setRumble(bool rumble) { _rumble = rumble; };

 private:
  void updateState();
  uint16_t convertToSwitchJoystick(uint8_t axisPos, double *minAxis,
                                   double *maxAxis);
  double _maxAnalogX = 0.5;
  double _minAnalogX = -0.5;
  double _maxAnalogY = 0.5;
  double _minAnalogY = -0.5;
  double _maxCX = 0.5;
  double _minCX = -0.5;
  double _maxCY = 0.5;
  double _minCY = -0.5;
};

#define GC_JOYSTICK_MIN 0x00
#define GC_JOYSTICK_MID 0x80
#define GC_JOYSTICK_MAX 0xFF

// GC First Byte
#define GC_MASK_A (0x1)
#define GC_MASK_B (0x1 << 1)
#define GC_MASK_X (0x1 << 2)
#define GC_MASK_Y (0x1 << 3)
#define GC_MASK_START (0x1 << 4)

// GC Second Byte
#define GC_MASK_DPAD (0xF)
#define GC_MASK_Z (0x1 << 4)
#define GC_MASK_R (0x1 << 5)
#define GC_MASK_L (0x1 << 6)
#define GC_MASK_DPAD_UP 0x8
#define GC_MASK_DPAD_UPRIGHT 0xA
#define GC_MASK_DPAD_RIGHT 0x2
#define GC_MASK_DPAD_DOWNRIGHT 0x6
#define GC_MASK_DPAD_DOWN 0x4
#define GC_MASK_DPAD_DOWNLEFT 0x5
#define GC_MASK_DPAD_LEFT 0x1
#define GC_MASK_DPAD_UPLEFT 0x9

#endif