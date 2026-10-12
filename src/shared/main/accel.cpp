#include "accel.h"

#include <string.h>

#include "Arduino.h"
#include "config.h"
#include "fxpt_math.h"
#include "io.h"
int16_t filtered[3] = {0};
uint16_t accel_adc[3] = {0};
double currentLowPassAlpha = LOW_PASS_ALPHA;
bool accel_found = false;
#ifdef ACCEL_TWI_PORT
static uint8_t lis3dh_address = LIS3DH_ADDRESS;
static uint8_t mpu6050_address = MPU6050_ADDRESS;
static uint8_t accel_type = NONE;
static uint32_t last_accel_probe = 0;
static void init_lis3dh() {
    uint8_t id = 0;
    lis3dh_address = LIS3DH_ADDRESS;
    twi_readFromPointer(ACCEL_TWI_PORT, lis3dh_address, LIS3DH_REG_WHOAMI, 1, &id);
    if (id != LIS3DH_ID && id != SC7A20_ID && id != LIS3DSH_ID) {
        lis3dh_address = LIS3DH_ADDRESS_2;
        twi_readFromPointer(ACCEL_TWI_PORT, lis3dh_address, LIS3DH_REG_WHOAMI, 1, &id);
        if (id != LIS3DH_ID && id != SC7A20_ID && id != LIS3DSH_ID) {
            return;
        }
    }
    accel_found = true;
    if (id == SC7A20_ID) {
        accel_type = SC7A20;
        twi_writeSingleToPointer(ACCEL_TWI_PORT, lis3dh_address, LIS3DH_REG_CTRL1, 0b01110111);  // enable all axis, set data rate to 4000hz
        twi_writeSingleToPointer(ACCEL_TWI_PORT, lis3dh_address, LIS3DH_REG_CTRL4, 0x88);        // High res & BDU enabled
    } else if (id == LIS3DSH_ID) {
        accel_type = LIS3DSH;
        twi_writeSingleToPointer(ACCEL_TWI_PORT, lis3dh_address, LIS3DH_REG_CTRL1, 0b01111111);  // enable all axis, BDU, set data rate to 4000hz
    } else {
        accel_type = LIS3DH;
        twi_writeSingleToPointer(ACCEL_TWI_PORT, lis3dh_address, LIS3DH_REG_CTRL1, 0b01110111);  // enable all axis, set data rate to 4000hz
        twi_writeSingleToPointer(ACCEL_TWI_PORT, lis3dh_address, LIS3DH_REG_CTRL4, 0x88);        // High res & BDU enabled
        twi_writeSingleToPointer(ACCEL_TWI_PORT, lis3dh_address, LIS3DH_REG_TEMPCFG, 0x80);      // enable adc
    }
}
static void init_adxl() {
    uint8_t id = 0;
    // Clones don't always report the right id, so accept anything that answers on the ADXL345 address
    if (!twi_readFromPointer(ACCEL_TWI_PORT, ADXL345_ADDRESS, ADXL345_REG_DEVID, 1, &id)) {
        return;
    }
    accel_type = ADXL345;
    accel_found = true;
    twi_writeSingleToPointer(ACCEL_TWI_PORT, ADXL345_ADDRESS, ADXL345_POWER_CTL, 0x08);
    twi_writeSingleToPointer(ACCEL_TWI_PORT, ADXL345_ADDRESS, ADXL345_DATA_FORMAT, 0x0B);
}
static bool probe_mpu6050(uint8_t address) {
    uint8_t id = 0;
    return twi_readFromPointerRepeatedStart(ACCEL_TWI_PORT, address, MPU6050_REG_WHO_AM_I, 1, &id) &&
           (id == MPU6050_ID || id == MPU6050_ID2 || id == MPU6050_ID3);
}

static void init_mpu6050() {
    uint8_t address = MPU6050_ADDRESS;
    if (!probe_mpu6050(address)) {
        address = MPU6050_ADDRESS_ALT;
        if (!probe_mpu6050(address)) {
            return;
        }
    }

    uint8_t power_management = 0;
    if (!twi_readFromPointerRepeatedStart(ACCEL_TWI_PORT, address, MPU6050_REG_PWR_MGMT_1, 1, &power_management)) {
        return;
    }

    power_management &= (uint8_t)~(MPU6050_PWR1_SLEEP | MPU6050_PWR1_CLKSEL_MASK);
    power_management |= MPU6050_PWR1_CLKSEL_XGYRO;
    if (!twi_writeSingleToPointer(ACCEL_TWI_PORT, address, MPU6050_REG_PWR_MGMT_1, power_management)) {
        return;
    }

    if (!twi_writeSingleToPointer(ACCEL_TWI_PORT, address, MPU6050_REG_ACCEL_CONFIG, MPU6050_ACCEL_CONFIG_2G)) {
        return;
    }

    mpu6050_address = address;
    accel_type = MPU6050;
    accel_found = true;
}
static void init_accel() {
    init_mpu6050();
    if (!accel_found) init_lis3dh();
    if (!accel_found) init_adxl();
}
void tick_accel() {
    int16_t raw[3];
    if (!accel_found) {
        memset(filtered, 0, sizeof(filtered));
        // Probing every address is slow, so don't retry on every tick when there is no accelerometer
        if (millis() - last_accel_probe < 500) {
            return;
        }
        last_accel_probe = millis();
        init_accel();
        return;
    }
    uint8_t shift = 0;
    switch (accel_type) {
        case LIS3DH:
            accel_found = twi_readFromPointer(ACCEL_TWI_PORT, lis3dh_address, LIS3DH_REG_OUT, 6, (uint8_t *)raw);
            twi_readFromPointer(ACCEL_TWI_PORT, lis3dh_address, LIS3DH_REG_OUTADC1_L, sizeof(accel_adc), (uint8_t *)accel_adc);
            break;
        case LIS3DSH:
            accel_found = twi_readFromPointer(ACCEL_TWI_PORT, lis3dh_address, LIS3DH_REG_OUT, 6, (uint8_t *)raw);
            break;
        case SC7A20:
            accel_found = twi_readFromPointer(ACCEL_TWI_PORT, lis3dh_address, LIS3DH_REG_OUT, 6, (uint8_t *)raw);
            shift = 4;
            break;
        case ADXL345:
            accel_found = twi_readFromPointer(ACCEL_TWI_PORT, ADXL345_ADDRESS, ADXL345_DATAX0, 6, (uint8_t *)raw);
            for (int i = 0; i < 3; i++) {
                raw[i] *= 64;
            }
            break;
        case MPU6050: {
            uint8_t raw_bytes[6];
            accel_found = twi_readFromPointerRepeatedStart(ACCEL_TWI_PORT, mpu6050_address, MPU6050_REG_ACCEL_OUT,
                                                           sizeof(raw_bytes), raw_bytes);
            for (int i = 0; i < 3; i++) {
                raw[i] = (int16_t)(((uint16_t)raw_bytes[i * 2] << 8) | raw_bytes[i * 2 + 1]);
            }
            break;
        }
        default:
            return;
    }
    if (!accel_found) {
        return;
    }
    for (int i = 0; i < 3; i++) {
        filtered[i] = (raw[i] >> shift) * currentLowPassAlpha + (filtered[i] * (1.0 - currentLowPassAlpha));
    }
}
#endif
