#include "mpu.h"
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h"

#define FSR 2000
//#define GYRO_SENS       ( 131.0f * 250.f / (float)FSR )
#define GYRO_SENS 16.375f
#define QUAT_SENS 1073741824.f // 2^30

#define EPSILON 0.0001f
#define PI_2 1.57079632679489661923f

struct s_mympu mympu;

struct s_quat {
  float w, x, y, z;
};

union u_quat {
  struct s_quat _f;
  long _l[4];
} q;

static int ret;
static short sensors;
static unsigned char fifoCount;
int mympu_open(unsigned int rate) {

  ret = mpu_init(NULL);
  ret += mpu_set_sensors(INV_XYZ_GYRO | INV_XYZ_ACCEL);
  ret += mpu_set_gyro_fsr(FSR);
  ret += mpu_set_accel_fsr(2);
  ret += mpu_configure_fifo(INV_XYZ_GYRO | INV_XYZ_ACCEL);
  ret += dmp_load_motion_driver_firmware();
  ret += dmp_set_fifo_rate(rate);
  ret += mpu_set_dmp_state(1);
  ret += dmp_enable_feature(DMP_FEATURE_6X_LP_QUAT);
  ret += dmp_set_interrupt_mode(DMP_INT_CONTINUOUS);
  return ret;
}

static float test, sqy, sqz, sqw;
static void quaternionToEuler(const struct s_quat *q, float *x, float *y,
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

static inline float wrap_pi(float x) {
  return (x < -M_PI ? x + M_PI + M_PI : (x > M_PI ? x - M_PI : x));
}

int mympu_update(void) {

  ret = dmp_read_fifo(NULL, NULL, q._l, NULL, &sensors, &fifoCount);
  if (ret != 0)
    return ret;

  q._f.w = (float)q._l[0] / (float)QUAT_SENS;
  q._f.x = (float)q._l[1] / (float)QUAT_SENS;
  q._f.y = (float)q._l[2] / (float)QUAT_SENS;
  q._f.z = (float)q._l[3] / (float)QUAT_SENS;

  quaternionToEuler(&q._f, &mympu.ypr[2], &mympu.ypr[1], &mympu.ypr[0]);
  mympu.ypr[0] = wrap_pi(mympu.ypr[0]);
  mympu.ypr[1] = wrap_pi(mympu.ypr[1]);
  mympu.ypr[2] = wrap_pi(mympu.ypr[2]);

  return 0;
}
