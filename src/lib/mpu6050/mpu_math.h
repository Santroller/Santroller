#pragma once

#include "math.h"
#include <stdint.h>
#define EPSILON 0.0001f
#define PI_2 1.57079632679489661923f
struct s_quat {
  int16_t w, x, y, z;
};

union u_quat {
  struct s_quat _f;
  long _l[4];
};

void quaternionToEuler(const struct s_quat *q, int16_t* out, uint8_t angle);