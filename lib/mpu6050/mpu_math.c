#include "mpu_math.h"
static float test, sqy, sqz, sqw;
void quaternionToEuler(const struct s_quat *q, float *x, float *y,
                              float *z) {

  test = q->x * q->z - q->w * q->y;

  if (test > 0.5f - EPSILON) {
    *x = 2.f * atan2(q->y, q->w);
    *y = PI_2;
    *z = 0;
  } else if (test < -0.5f + EPSILON) {
    *x = -2.f * atan2(q->y, q->w);
    *y = -PI_2;
    *z = 0;
  } else {
    sqy = q->y * q->y;
    sqz = q->z * q->z;
    sqw = q->w * q->w;
    *x = atan2(2.f * (q->x * q->w + q->y * q->z), 1.f - 2.f * (sqz + sqw));
    *y = asin(2.f * test);
    *z = atan2(2.f * (q->x * q->y - q->z * q->w), 1.f - 2.f * (sqy + sqz));
  }
}

float wrap_pi(float x) {
  return (x < -M_PI ? x + M_PI + M_PI : (x > M_PI ? x - M_PI : x));
}