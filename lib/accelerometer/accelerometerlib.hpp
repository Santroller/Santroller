// Support for LIS3DH, MPU-6050 and ADXL345 accelerometers
#pragma once
#include "i2c.hpp"
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
#define LIS3DH_REG_CTRL2 0x21
#define LIS3DH_REG_CTRL3 0x22
#define LIS3DH_REG_CTRL4 0x23
#define LIS3DH_REG_WHOAMI 0x0F
#define LIS3DH_REG_STATUS 0x27
#define LIS3DH_REG_STATUS_AUX 0x07
#define LIS3DH_REG_OUT 0x28 | 0x80  // Set auto increment
#define LIS3DH_REG_OUTADC1_L 0x08 | 0x80
#define LIS3DH_REG_OUTADC1_H 0x09 | 0x80
#define LIS3DH_REG_OUTADC2_L 0x0A | 0x80
#define LIS3DH_REG_OUTADC2_H 0x0B | 0x80
#define LIS3DH_REG_OUTADC3_L 0x0C | 0x80
#define LIS3DH_REG_OUTADC3_H 0x0D | 0x80
#define LIS3DH_ID 0x33
#define LIS3DSH_ID 0x3F
#define SC7A20_ID 0x11
#define LIS3DH_REG_TEMPCFG 0x1F
#define MPU6050_ADDRESS 0x68
#define MPU6050_REG_ACCEL_OUT 0x3B
#define MPU6050_REG_WHO_AM_I 0x75
#define MPU6050_REG_ACCEL_CONFIG 0x1C
#define MPU6050_REG_PWR_MGMT_1 0x6B
#define MPU6050_ID 0x68
#define MPU6050_FAKE_ID 0x98
enum AccelerometerType {
    None,
    ADXL345,
    LIS3DH,
    MPU6050,
    SC7A20,
    LIS3DSH
};
typedef enum {
    ACCEL_INIT,
    SC7A20_CTRL1_INIT,
    SC7A20_CTRL4_INIT,
    LIS3DH_CTRL1_INIT,
    LIS3DH_CTRL4_INIT,
    LIS3DH_TEMPCFG_INIT,
    LIS3DSH_CTRL1_INIT,
    ADXL_POWERCTL,
    ADXL_DATAFORMAT,
    MPU_6050_ACCEL_CONFIG,
    MPU_6050_PWR_MGMT_1_READ,
    MPU_6050_PWR_MGMT_1_WRITE,
    LIS_FAMILY_POLL,
    LIS3DH_POLL,
    LIS3DH_POLL_AUX,
    ADXL_POLL,
    MPU_6050_POLL
} accel_status_e;
class Accelerometer: public I2CDMAInterface {
   public:
    Accelerometer(uint8_t block, uint8_t sda, uint8_t scl, uint32_t clock)
        : interface(block, sda, scl, clock) {};
    void tick();
    void begin();
    void end();
    inline bool is_connected() {
        return status != ACCEL_INIT;
    }
    uint16_t lis3dhAdc[3];
    int32_t accel[3];
    void processData(uint8_t addr, bool running, bool timeout, bool abort_detected, bool stop_detected);

   private:
    I2CMasterInterface interface;
    AccelerometerType type;
    uint8_t address;
    uint8_t reg;
    int currentPoll=0;
    int failCount = 0;
    alarm_id_t restart_alarm_id;
    accel_status_e status = ACCEL_INIT;
    uint8_t bufferTxLis1[1];
    uint8_t bufferRxLis1[1];
    uint8_t bufferTxLis2[1];
    uint8_t bufferRxLis2[1];
    uint8_t bufferTxAdxl[1];
    uint8_t bufferRxAdxl[1];
    uint8_t bufferTxMpu[1];
    uint8_t bufferRxMpu[1];
    uint8_t bufferTx[32];
    uint8_t bufferRx[32];
    bool seen_response_lis3dh_1 = true;
    bool seen_response_lis3dh_2 = true;
    bool seen_response_adxl345 = true;
    bool seen_response_mpu6050 = true;
};
