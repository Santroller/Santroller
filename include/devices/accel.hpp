#pragma once
#include "interfaces/i2c.hpp"
#include "state/base.hpp"
#define ADXL345_ADDRESS 0x53
#define ADXL345_POWER_CTL 0x2D
#define ADXL345_DATA_FORMAT 0x31
#define ADXL345_DATAX0 0x32
#define ADXL345_GRAVITY_EARTH 9.80665f
#define ADXL345_ID 0xE5
#define ADXL345_REG_DEVID 0x00
#define LIS3DH_ADDRESS 0x18
#define LIS3DH_ADDRESS_2 0x19
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
enum AccelerometerType {
    None,
    ADXL345,
    LIS3DH,
    MPU6050
};
class AccelerometerDevice {
   public:
    inline AccelerometerDevice(I2CMasterInterface* interface) : mInterface(interface), mType(None), mAddress(0), mReg(0), mFound(false) {
    }
    void tick(san_base_t* data);
    void initLis3dh();
    void initMPU6050();
    void initADXL345();

   private:
    I2CMasterInterface* mInterface;
    AccelerometerType mType;
    uint8_t mAddress;
    uint8_t mReg;
    bool mFound;
};