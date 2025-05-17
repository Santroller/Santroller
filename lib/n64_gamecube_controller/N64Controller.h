#ifndef N64Controller_h
#define N64Controller_h

#include "Controller.h"

class N64Controller : public Controller {
 public:
  N64Controller(InitParams *initParams) : Controller(initParams, 4){};
  void init();
  void setRumble(bool rumble);

 private:
  void updateState();
  uint16_t convertToSwitchJoystick(int8_t axisPos, double *minAxis,
                                   double *maxAxis);
  void initController();
  void writeRumble(uint8_t writeVal);
  double _maxX = 0.5;
  double _minX = -0.5;
  double _maxY = 0.5;
  double _minY = -0.5;
};

#define N64_JOYSTICK_MAX 0x80
#define N64_C_OFFSET 16

// First byte
#define N64_MASK_A (0x1 << 7)
#define N64_MASK_B (0x1 << 6)
#define N64_MASK_Z (0x1 << 5)
#define N64_MASK_START (0x1 << 4)
#define N64_MASK_DPAD 0xF
#define N64_MASK_RESET (0x1 << 7)
#define N64_MASK_L (0x1 << 5)
#define N64_MASK_R (0x1 << 4)
#define N64_MASK_C 0xF

#define N64_MASK_DPAD_UP 0x8
#define N64_MASK_DPAD_UPRIGHT 0x9
#define N64_MASK_DPAD_RIGHT 0x1
#define N64_MASK_DPAD_DOWNRIGHT 0x5
#define N64_MASK_DPAD_DOWN 0x4
#define N64_MASK_DPAD_DOWNLEFT 0x6
#define N64_MASK_DPAD_LEFT 0x2
#define N64_MASK_DPAD_UPLEFT 0xA

#define N64_MASK_C_UP 0x8
#define N64_MASK_C_UPRIGHT 0x9
#define N64_MASK_C_RIGHT 0x1
#define N64_MASK_C_DOWNRIGHT 0x5
#define N64_MASK_C_DOWN 0x4
#define N64_MASK_C_DOWNLEFT 0x6
#define N64_MASK_C_LEFT 0x2
#define N64_MASK_C_UPLEFT 0xA
#define N64_MASK_C_UPDOWN 0xC
#define N64_MASK_C_LEFTRIGHT 0x3
#define N64_MASK_C_UPDOWNLEFT 0xE
#define N64_MASK_C_UPDOWNRIGHT 0xD
#define N64_MASK_C_UPLEFTRIGHT 0xB
#define N64_MASK_C_DOWNLEFTRIGHT 0x7
#define N64_MASK_C_UPDOWNLEFTRIGHT 0xF

#endif