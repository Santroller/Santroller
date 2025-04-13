#include "devices/accel.hpp"

void AccelerometerDevice::initLis3dh() {
    if (mFound) {
        return;
    }
    uint8_t id = 0;
    mAddress = LIS3DH_ADDRESS;
    mInterface->readFromPointer(mAddress, LIS3DH_REG_WHOAMI, 1, &id);
    if (id != LIS3DH_ID) {
        mAddress = LIS3DH_ADDRESS_2;
        mInterface->readFromPointer(mAddress, LIS3DH_REG_WHOAMI, 1, &id);
        if (id != LIS3DH_ID) {
            return;
        }
    }
    mType = LIS3DH;
    mFound = true;
    mInterface->writeSingleToPointer(mAddress, LIS3DH_REG_CTRL1, 0b01110111);  // enable all axis, set data rate to 4000hz
    mInterface->writeSingleToPointer(mAddress, LIS3DH_REG_CTRL4, 0x88);        // High res & BDU enabled
    mInterface->writeSingleToPointer(mAddress, LIS3DH_REG_TEMPCFG, 0x80);      // enable adc
    mReg = MPU6050_REG_ACCEL_OUT;
}

void AccelerometerDevice::initMPU6050() {
    if (mFound) {
        return;
    }
    mAddress = MPU6050_ADDRESS;
    uint8_t id = 0;
    mInterface->readFromPointer(mAddress, MPU6050_REG_WHO_AM_I, 1, &id);
    if (id != MPU6050_ID) {
        return;
    }
    mType = MPU6050;
    mFound = true;
    mInterface->writeSingleToPointer(mAddress, MPU6050_REG_ACCEL_CONFIG, 0b11100111);
    mInterface->readFromPointer(mAddress, MPU6050_REG_PWR_MGMT_1, 1, &id);
    id &= ~(1 << 6);
    mInterface->writeSingleToPointer(mAddress, MPU6050_REG_PWR_MGMT_1, id);
    mReg = LIS3DH_REG_OUT;
}

void AccelerometerDevice::initADXL345() {
    if (mFound) {
        return;
    }
    mAddress = ADXL345_ADDRESS;
    uint8_t id = 0;
    mInterface->readFromPointer(mAddress, ADXL345_REG_DEVID, 1, &id);
    if (id != ADXL345_ID) {
        return;
    }
    mType = ADXL345;
    mFound = true;
    mReg = ADXL345_DATAX0;
    mInterface->writeSingleToPointer(mAddress, ADXL345_POWER_CTL, 0x08);
    mInterface->writeSingleToPointer(mAddress, ADXL345_DATA_FORMAT, 0x0B);
}

void AccelerometerDevice::tick(san_base_t* data) {
    if (!mFound) {
        initADXL345();
        initMPU6050();
        initLis3dh();
        return;
    }
    int16_t raw[3];
    mFound = mInterface->readFromPointer(mAddress, mReg, 6, (uint8_t*)raw);
    for (uint8_t i = 0; i < 3; i++) {
        data->gamepad.accel[i] = raw[i];
        // ADXL345 needs to be scaled
        if (mType == ADXL345) {
            data->gamepad.accel[i] *= 64;
        }
    }
    if (mType == LIS3DH) {
        mFound = mInterface->readFromPointer(mAddress, LIS3DH_REG_OUTADC1_L, sizeof(data->extra.lis3dhAdc), (uint8_t*)data->extra.lis3dhAdc);
    }
}