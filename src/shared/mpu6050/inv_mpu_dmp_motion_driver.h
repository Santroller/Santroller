/*
 $License:
    Copyright (C) 2011-2012 InvenSense Corporation, All Rights Reserved.
    See included License.txt for License information.
 $
 */
/**
 *  @addtogroup  DRIVERS Sensor Driver Layer
 *  @brief       Hardware drivers to communicate with sensors via I2C.
 *
 *  @{
 *      @file       inv_mpu_dmp_motion_driver.h
 *      @brief      DMP image and interface functions.
 *      @details    All functions are preceded by the dmp_ prefix to
 *                  differentiate among MPL and general driver function calls.
 */
#ifndef _INV_MPU_DMP_MOTION_DRIVER_H_
#define _INV_MPU_DMP_MOTION_DRIVER_H_

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define TAP_X (0x01)
#define TAP_Y (0x02)
#define TAP_Z (0x04)
#define TAP_XYZ (0x07)

#define TAP_X_UP (0x01)
#define TAP_X_DOWN (0x02)
#define TAP_Y_UP (0x03)
#define TAP_Y_DOWN (0x04)
#define TAP_Z_UP (0x05)
#define TAP_Z_DOWN (0x06)

#define ANDROID_ORIENT_PORTRAIT (0x00)
#define ANDROID_ORIENT_LANDSCAPE (0x01)
#define ANDROID_ORIENT_REVERSE_PORTRAIT (0x02)
#define ANDROID_ORIENT_REVERSE_LANDSCAPE (0x03)

#define DMP_INT_GESTURE (0x01)
#define DMP_INT_CONTINUOUS (0x02)

#define DMP_FEATURE_TAP (0x001)
#define DMP_FEATURE_ANDROID_ORIENT (0x002)
#define DMP_FEATURE_LP_QUAT (0x004)
#define DMP_FEATURE_PEDOMETER (0x008)
#define DMP_FEATURE_6X_LP_QUAT (0x010)
#define DMP_FEATURE_GYRO_CAL (0x020)
#define DMP_FEATURE_SEND_RAW_ACCEL (0x040)
#define DMP_FEATURE_SEND_RAW_GYRO (0x080)
#define DMP_FEATURE_SEND_CAL_GYRO (0x100)

#define INV_WXYZ_QUAT (0x100)

/* Set up functions. */
int8_t dmp_load_motion_driver_firmware(void);
int8_t dmp_set_fifo_rate(unsigned short rate);
int8_t dmp_get_fifo_rate(unsigned short *rate);
int8_t dmp_enable_feature(unsigned short mask);
int8_t dmp_get_enabled_features(unsigned short *mask);
int8_t dmp_set_interrupt_mode(unsigned char mode);
int8_t dmp_set_orientation(unsigned short orient);
int8_t dmp_set_gyro_bias(long *bias);
int8_t dmp_set_accel_bias(long *bias);

/* LP quaternion functions. */
int8_t dmp_enable_lp_quat(unsigned char enable);
int8_t dmp_enable_6x_lp_quat(unsigned char enable);

/* DMP gyro calibration functions. */
int8_t dmp_enable_gyro_cal(unsigned char enable);

/* Read function. This function should be called whenever the MPU interrupt is
 * detected.
 */
int8_t dmp_read_fifo(short *gyro, short *accel, long *quat,
                     unsigned long *timestamp, short *sensors,
                     unsigned char *more);

#ifdef MPU_EXTRAS
/* Tap functions. */
int8_t dmp_register_tap_cb(void (*func)(unsigned char, unsigned char));
int8_t dmp_set_tap_thresh(unsigned char axis, unsigned short thresh);
int8_t dmp_set_tap_axes(unsigned char axis);
int8_t dmp_set_tap_count(unsigned char min_taps);
int8_t dmp_set_tap_time(unsigned short time);
int8_t dmp_set_tap_time_multi(unsigned short time);
int8_t dmp_set_shake_reject_thresh(long sf, unsigned short thresh);
int8_t dmp_set_shake_reject_time(unsigned short time);
int8_t dmp_set_shake_reject_timeout(unsigned short time);

/* Android orientation functions. */
int8_t dmp_register_android_orient_cb(void (*func)(unsigned char));

/* Pedometer functions. */
int8_t dmp_get_pedometer_step_count(unsigned long *count);
int8_t dmp_set_pedometer_step_count(unsigned long count);
int8_t dmp_get_pedometer_walk_time(unsigned long *time);
int8_t dmp_set_pedometer_walk_time(unsigned long time);
#endif // MPU_EXTRAS

#ifdef __cplusplus
}
#endif
#endif // #ifndef _INV_MPU_DMP_MOTION_DRIVER_H_