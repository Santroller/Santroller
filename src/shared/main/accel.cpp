#include "accel.h"

#include "config.h"
#include "fxpt_math.h"
#include "io.h"
int16_t filtered[3] = {0};
uint16_t accel_adc[3] = {0};
double currentLowPassAlpha = LOW_PASS_ALPHA;
bool accel_found = false;
uint8_t lis3dh_address = LIS3DH_ADDRESS;
uint8_t type = NONE;
#ifdef ACCEL_TWI_PORT
void init_lis3dh() {
    uint8_t id = 0;
    lis3dh_address = LIS3DH_ADDRESS;
    twi_readFromPointer(ACCEL_TWI_PORT, lis3dh_address, LIS3DH_REG_WHOAMI, 1, &id);
    if (id != LIS3DH_ID) {
        lis3dh_address = LIS3DH_ADDRESS_2;
        twi_readFromPointer(ACCEL_TWI_PORT, lis3dh_address, LIS3DH_REG_WHOAMI, 1, &id);
        if (id != LIS3DH_ID) {
            return;
        }
    }
    type = LIS3DH;
    accel_found = true;
    twi_writeSingleToPointer(ACCEL_TWI_PORT, lis3dh_address, LIS3DH_REG_CTRL1, 0b01110111);  // enable all axis, set data rate to 4000hz
    twi_writeSingleToPointer(ACCEL_TWI_PORT, lis3dh_address, LIS3DH_REG_CTRL4, 0x88);        // High res & BDU enabled
    twi_writeSingleToPointer(ACCEL_TWI_PORT, lis3dh_address, LIS3DH_REG_TEMPCFG, 0x80);      // enable adc
}
void init_adxl() {
    uint8_t id = 0;
    twi_readFromPointer(ACCEL_TWI_PORT, ADXL345_ADDRESS, ADXL345_REG_DEVID, 1, &id);
    if (id != ADXL345_ID) {
        return;
    }
    type = ADXL345;
    accel_found = true;
    twi_writeSingleToPointer(ACCEL_TWI_PORT, ADXL345_ADDRESS, ADXL345_POWER_CTL, 0x08);
    twi_writeSingleToPointer(ACCEL_TWI_PORT, ADXL345_ADDRESS, ADXL345_DATA_FORMAT, 0x0B);
}
void init_mpu6050() {
    uint8_t id = 0;
    twi_readFromPointer(ACCEL_TWI_PORT, MPU6050_ADDRESS, MPU6050_REG_WHO_AM_I, 1, &id);
    if (id != MPU6050_ID) {
        return;
    }
    type = MPU6050;
    accel_found = true;
    twi_writeSingleToPointer(ACCEL_TWI_PORT, MPU6050_ADDRESS, MPU6050_REG_ACCEL_CONFIG, 0b11100111);
    twi_readFromPointer(ACCEL_TWI_PORT, MPU6050_ADDRESS, MPU6050_REG_PWR_MGMT_1, 1, &id);
    id &= ~(1 << 6);
    twi_writeSingleToPointer(ACCEL_TWI_PORT, MPU6050_ADDRESS, MPU6050_REG_PWR_MGMT_1, id);
}
void init_accel() {
    init_lis3dh();
    init_adxl();
    init_mpu6050();
}
void tick_accel() {
    if (!accel_found) {
        init_accel();
        return;
    }
    int16_t raw[3];
    if (type == LIS3DH) {
        accel_found = twi_readFromPointer(ACCEL_TWI_PORT, lis3dh_address, LIS3DH_REG_OUT, 6, (uint8_t*)raw);
        accel_found = twi_readFromPointer(ACCEL_TWI_PORT, lis3dh_address, LIS3DH_REG_OUTADC1_L, sizeof(accel_adc), (uint8_t*)accel_adc);
        for (int i = 0; i < 3; i++) {
            filtered[i] = (raw[i]) * currentLowPassAlpha + (filtered[i] * (1.0 - currentLowPassAlpha));
        }
    }
    if (type == ADXL345) {
        accel_found = twi_readFromPointer(ACCEL_TWI_PORT, ADXL345_ADDRESS, ADXL345_DATAX0, 6, (uint8_t*)raw);
        for (int i = 0; i < 3; i++) {
            filtered[i] = (raw[i] * 64) * currentLowPassAlpha + (filtered[i] * (1.0 - currentLowPassAlpha));
        }
    }
    if (type == MPU6050) {
        accel_found = twi_readFromPointer(ACCEL_TWI_PORT, MPU6050_ADDRESS, MPU6050_REG_ACCEL_OUT, 6, (uint8_t*)raw);
        for (int i = 0; i < 3; i++) {
            filtered[i] = (raw[i]) * currentLowPassAlpha + (filtered[i] * (1.0 - currentLowPassAlpha));
        }
    }
}
#endif