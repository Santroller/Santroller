#pragma once

#include "math.h"
#define EPSILON 0.0001f
#define PI_2 1.57079632679489661923f
// Constants used by the mpu 6050
#define FSR 2000
//#define GYRO_SENS       ( 131.0f * 250.f / (float)FSR )
#define GYRO_SENS 16.375f
#define QUAT_SENS 1073741824.f // 2^30
struct s_quat {
  float w, x, y, z;
};

union u_quat {
  struct s_quat _f;
  long _l[4];
};

float wrap_pi(float x);
void quaternionToEuler(const struct s_quat *q, float *x, float *y, float *z);