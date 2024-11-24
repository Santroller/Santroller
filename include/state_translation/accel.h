#include <stdint.h>

#include "Arduino.h"
#include "commands.h"
#include "config.h"
#include "defines.h"
#include "fxpt_math.h"
#include "io.h"
#include "reports/controller_reports.h"
#include "state_translation/shared.h"
#include "state_translation/slider.h"
#define ADXL345_ADDRESS 0x53
#define ADXL345_POWER_CTL 0x2D
#define ADXL345_DATA_FORMAT 0x31
#define ADXL345_DATAX0 0x32
#define ADXL345_GRAVITY_EARTH 9.80665f
#define ADXL345_ID 0xE5
#define ADXL345_REG_DEVID 0x00
#define LIS3DH_ADDRESS 0x18
#define LIS3DH_REG_CTRL1 0x20
#define LIS3DH_REG_CTRL4 0x23
#define LIS3DH_REG_WHOAMI 0x0F
#define LIS3DH_REG_OUT 0x28 | 0x80  // Set auto increment
#define LIS3DH_REG_OUTADC1_L 0x08 | 0x80
#define LIS3DH_REG_OUTADC1_H 0x09 | 0x80
#define LIS3DH_REG_OUTADC2_L 0x0A | 0x80
#define LIS3DH_REG_OUTADC2_H 0x0B | 0x80
#define LIS3DH_REG_OUTADC3_L 0x0C | 0x80
#define LIS3DH_REG_OUTADC3_H 0x0D | 0x80
#define LIS3DH_ID 0x33
#define LIS3DH_REG_TEMPCFG 0x1F
#define MPU6050_ADDRESS 0x68
#define MPU6050_REG_ACCEL_OUT 0x3B
#define MPU6050_REG_WHO_AM_I 0x75
#define MPU6050_REG_ACCEL_CONFIG 0x1C
#define MPU6050_REG_PWR_MGMT_1 0x6B
#define MPU6050_ID 0x68
inline void accel_to_universal_report(USB_Host_Data_t *usb_host_data) {
    static int16_t filtered[3] = {0};
    static uint16_t accel_adc[3] = {0};
    static double currentLowPassAlpha = LOW_PASS_ALPHA;
    static bool accel_found = false;
    if (!accel_found) {
        switch (ACCEL_TYPE) {
            case LIS3DH:
                uint8_t id = 0;
                twi_readFromPointer(ACCEL_TWI_PORT, LIS3DH_ADDRESS, LIS3DH_REG_WHOAMI, 1, &id);
                if (id != LIS3DH_ID) {
                    return;
                }
                accel_found = true;
                twi_writeSingleToPointer(ACCEL_TWI_PORT, LIS3DH_ADDRESS, LIS3DH_REG_CTRL1, 0b01110111);  // enable all axis, set data rate to 4000hz
                twi_writeSingleToPointer(ACCEL_TWI_PORT, LIS3DH_ADDRESS, LIS3DH_REG_CTRL4, 0x88);        // High res & BDU enabled
                twi_writeSingleToPointer(ACCEL_TWI_PORT, LIS3DH_ADDRESS, LIS3DH_REG_TEMPCFG, 0x80);      // enable adc
                break;
            case ADXL345:
                uint8_t id = 0;
                twi_readFromPointer(ACCEL_TWI_PORT, ADXL345_ADDRESS, ADXL345_REG_DEVID, 1, &id);
                if (id != ADXL345_ID) {
                    return;
                }
                accel_found = true;
                twi_writeSingleToPointer(ACCEL_TWI_PORT, ADXL345_ADDRESS, ADXL345_POWER_CTL, 0x08);
                twi_writeSingleToPointer(ACCEL_TWI_PORT, ADXL345_ADDRESS, ADXL345_DATA_FORMAT, 0x0B);
                break;
            case MPU6050:
                uint8_t id = 0;
                twi_readFromPointer(ACCEL_TWI_PORT, MPU6050_ADDRESS, MPU6050_REG_WHO_AM_I, 1, &id);
                if (id != MPU6050_ID) {
                    return;
                }
                accel_found = true;
                twi_writeSingleToPointer(ACCEL_TWI_PORT, MPU6050_ADDRESS, MPU6050_REG_ACCEL_CONFIG, 0b11100111);
                twi_readFromPointer(ACCEL_TWI_PORT, MPU6050_ADDRESS, MPU6050_REG_PWR_MGMT_1, 1, &id);
                id &= ~(1 << 6);
                twi_writeSingleToPointer(ACCEL_TWI_PORT, MPU6050_ADDRESS, MPU6050_REG_PWR_MGMT_1, id);
                break;
        }
        return;
    }
    int16_t raw[3];
    switch (ACCEL_TYPE) {
        case LIS3DH:
            accel_found = twi_readFromPointer(ACCEL_TWI_PORT, LIS3DH_ADDRESS, LIS3DH_REG_OUT, 6, (uint8_t *)raw);
            accel_found = twi_readFromPointer(ACCEL_TWI_PORT, LIS3DH_ADDRESS, LIS3DH_REG_OUTADC1_L, sizeof(accel_adc), (uint8_t *)accel_adc);
            for (int i = 0; i < 3; i++) {
                filtered[i] = (raw[i]) * currentLowPassAlpha + (filtered[i] * (1.0 - currentLowPassAlpha));
            }
            break;
        case ADXL345:
            accel_found = twi_readFromPointer(ACCEL_TWI_PORT, ADXL345_ADDRESS, ADXL345_DATAX0, 6, (uint8_t *)raw);
            for (int i = 0; i < 3; i++) {
                filtered[i] = (raw[i] * 64) * currentLowPassAlpha + (filtered[i] * (1.0 - currentLowPassAlpha));
            }
            break;
        case MPU6050:
            accel_found = twi_readFromPointer(ACCEL_TWI_PORT, MPU6050_ADDRESS, MPU6050_REG_ACCEL_OUT, 6, (uint8_t *)raw);
            for (int i = 0; i < 3; i++) {
                filtered[i] = (raw[i]) * currentLowPassAlpha + (filtered[i] * (1.0 - currentLowPassAlpha));
            }
            break;
    }
    usb_host_data->accelX = filtered[0];
    usb_host_data->accelY = filtered[1];
    usb_host_data->accelZ = filtered[2];
}