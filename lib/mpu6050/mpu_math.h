#pragma once

#include "math.h"
#define EPSILON 0.0001f
#define PI_2 1.57079632679489661923f
struct s_quat {
  float w, x, y, z;
};

union u_quat {
  struct s_quat _f;
  long _l[4];
  float _f2[4];
};

float wrap_pi(float x);
void quaternionToEuler(const struct s_quat *q, float *x, float *y, float *z);