#pragma once
#include "../controller/controller.h"
#define I2C_ADDR 0x52
#define NUNCHUK             0x0000
#define CLASSIC             0x0101
#define CLASSIC_PRO         0x1101
#define UDRAW               0xF012
#define DRAWSOME            0xF013
#define GUITAR              0x0103
#define DRUMS               0x1103
#define TURNTABLE           0x3103
#define TATACON             0x0111
#define MOTION_PLUS         0x0405 
#define MOTION_PLUS_NUNCHUK 0x0505
#define MOTION_PLUS_CLASSIC 0x0705
#define NO_DEVICE           0xFFFF
void wii_ext_init(void);
void wii_ext_tick(controller_t* controller);
void get_wii_device_name(char* buf);