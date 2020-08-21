#include "mpu_math.h"
#include "fxpt_atan2.h"
#include "config/defines.h"
#define MUL 5
#define MAX 32767/MUL
static int16_t test, sqy, sqz, sqw;

void quaternionToEuler(const struct s_quat *q, int16_t *out, uint8_t angle) {

  test = q->x * q->z - q->w * q->y;
  sqy = q->y * q->y;
  sqz = q->z * q->z;
  sqw = q->w * q->w;
  switch (angle) {
    case NEGATIVE_X:
      *out = -fxpt_atan2(2 * (q->x * q->y - q->z * q->w), 32768 - 2 * (sqy + sqz));
      break;
    case POSITIVE_X:
      *out = fxpt_atan2(2 * (q->x * q->y - q->z * q->w), 32768 - 2 * (sqy + sqz));
      break;
    case NEGATIVE_Y:
      *out = -fxpt_asin(test);
      break;
    case POSITIVE_Y:
      *out = fxpt_asin(test);
      break;
    case NEGATIVE_Z:
      *out = -fxpt_atan2(2 * (q->x * q->w + q->y * q->z), 32768 - 2 * (sqz + sqw));
      break;
    case POSITIVE_Z:
      *out = fxpt_atan2(2 * (q->x * q->w + q->y * q->z), 32768 - 2 * (sqz + sqw));
      break;
  }
  if (*out > MAX) {
    *out = MAX;
  }
  if (*out < -MAX) {
    *out = -MAX;
  }
  *out *= MUL;
}