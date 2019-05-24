#pragma once
#include "../../io/twi/I2Cdev.h"
#include "../../util.h"
#include "../Controller.h"
#include "Input.h"
#include "../../config/defines.h"
#include <util/delay.h>
#define I2C_ADDR 0x52
#define NUNCHUK             0x0000
#define CLASSIC             0x0101
#define CLASSIC_PRO         0x1101
#define UDRAW               0xF012
#define DRAWSOME            0xF013
#define GUITAR              0x0103
#define GUITAR_2            0x0003 
#define DRUMS               0x1103
#define TURNTABLE           0x3103
#define MOTION_PLUS         0x0405 
#define MOTION_PLUS_NUNCHUK 0x0505
#define MOTION_PLUS_CLASSIC 0x0705
class WiiExtension : public Input {
public:
  void read_controller(Controller *data);
  void init();
  short counter;
  uint16_t id;

private:
  uint16_t read_ext_id();
  void init_controller();
};