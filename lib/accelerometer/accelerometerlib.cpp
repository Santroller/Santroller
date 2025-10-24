#include "accelerometerlib.hpp"
#include "stdio.h"
#include "utils.h"
#include "hardware/gpio.h"
void Accelerometer::initLis3dh()
{
    if (connected)
    {
        return;
    }
    uint8_t id = 0;
    address = LIS3DH_ADDRESS;
    interface.readRegister(address, LIS3DH_REG_WHOAMI, 1, &id);
    if (id != LIS3DH_ID)
    {
        address = LIS3DH_ADDRESS_2;
        interface.readRegister(address, LIS3DH_REG_WHOAMI, 1, &id);
        if (id != LIS3DH_ID)
        {
            return;
        }
    }
    type = LIS3DH;
    connected = true;
    interface.writeRegister(address, LIS3DH_REG_CTRL1, 0b01110111); // enable all axis, set data rate to 4000hz
    interface.writeRegister(address, LIS3DH_REG_CTRL4, 0x88);       // High res & BDU enabled
    interface.writeRegister(address, LIS3DH_REG_TEMPCFG, 0x80);     // enable adc
    reg = LIS3DH_REG_OUT;
}

void Accelerometer::initMPU6050()
{
    if (connected)
    {
        return;
    }
    address = MPU6050_ADDRESS;
    uint8_t id = 0;
    interface.readRegister(address, MPU6050_REG_WHO_AM_I, 1, &id);
    if (id != MPU6050_ID)
    {
        return;
    }
    type = MPU6050;
    connected = true;
    interface.writeRegister(address, MPU6050_REG_ACCEL_CONFIG, 0b11100111);
    interface.readRegister(address, MPU6050_REG_PWR_MGMT_1, 1, &id);
    id &= ~(1 << 6);
    interface.writeRegister(address, MPU6050_REG_PWR_MGMT_1, id);
    reg = MPU6050_REG_ACCEL_OUT;
}

void Accelerometer::initADXL345()
{
    if (connected)
    {
        return;
    }
    address = ADXL345_ADDRESS;
    uint8_t id = 0;
    interface.readRegister(address, ADXL345_REG_DEVID, 1, &id);
    if (id != ADXL345_ID)
    {
        return;
    }
    type = ADXL345;
    connected = true;
    reg = ADXL345_DATAX0;
    interface.writeRegister(address, ADXL345_POWER_CTL, 0x08);
    interface.writeRegister(address, ADXL345_DATA_FORMAT, 0x0B);
}

void Accelerometer::tick()
{
    if (!connected)
    {
        initADXL345();
        initMPU6050();
        initLis3dh();
        return;
    }
    // Poll accelerometers one channel per poll, so we can make sure they won't ever slow down our poll rate
    if ((type != LIS3DH && currentPoll >= 3) || currentPoll >= 6)
    {
        currentPoll = 0;
    }
    if (currentPoll < 3)
    {
        int16_t raw;
        connected = interface.readRegister(address, reg + (currentPoll * 2), 2, (uint8_t *)&raw);
        accel[currentPoll] = raw;
        // ADXL345 needs to be scaled
        if (type == ADXL345)
        {
            accel[currentPoll] *= 64;
        }
    }
    else
    {
        uint16_t raw;
        connected = interface.readRegister(address, (LIS3DH_REG_OUTADC1_L) + (currentPoll - 3) * 2, 2, (uint8_t *)&raw);
        lis3dhAdc[currentPoll - 3] = raw;
    }
    currentPoll++;
}