#include "accelerometerlib.hpp"
#include "stdio.h"
#include "utils.h"
#include "hardware/gpio.h"
static int64_t restart_handler(__unused alarm_id_t id, void *user_data)
{
    Accelerometer *inst = (Accelerometer *)user_data;
    if (inst)
    {
        inst->processData(0, false, false, false, false);
    }
    return 0;
}

void Accelerometer::begin()
{
    printf("accel begin\r\n");
    interface.dmaInit(ADXL345_ADDRESS, this);
    interface.dmaInit(MPU6050_ADDRESS, this);
    interface.dmaInit(LIS3DH_ADDRESS, this);
    interface.dmaInit(LIS3DH_ADDRESS_2, this);
    seen_response_lis3dh_1 = true;
    seen_response_lis3dh_2 = true;
    seen_response_adxl345 = true;
    seen_response_mpu6050 = true;
    status = ACCEL_INIT;
    processData(0, false, false, false, false);
}
void Accelerometer::end()
{
    printf("accel end\r\n");
    cancel_alarm(restart_alarm_id);
    interface.dmaDeinit(ADXL345_ADDRESS);
    interface.dmaDeinit(MPU6050_ADDRESS);
    interface.dmaDeinit(LIS3DH_ADDRESS);
    interface.dmaDeinit(LIS3DH_ADDRESS_2);
}
void Accelerometer::processData(uint8_t addr, bool running, bool timeout, bool abort_detected, bool stop_detected)
{
    // printf("processData %02x %d %d %d %d %d\r\n", addr, status, running, timeout, abort_detected, stop_detected);
    // If we have started init, ignore i2c data from the other accelerometers
    if (status != ACCEL_INIT && addr && addr != address)
    {
        return;
    }
    cancel_alarm(restart_alarm_id);
    if (timeout || abort_detected)
    {
        if (status != ACCEL_INIT)
        {
            failCount++;
        }
        // during high load, there might be the occassional drop, so allow a few failures
        if (failCount > 10 || status == ACCEL_INIT)
        {
            status = ACCEL_INIT;
            type = AccelerometerType::None;
        }
        restart_alarm_id = add_alarm_in_ms(500, restart_handler, this, true);
        return;
    }
    if (stop_detected)
    {
        seen_response_lis3dh_1 = addr == LIS3DH_ADDRESS;
        seen_response_lis3dh_2 = addr == LIS3DH_ADDRESS_2;
        seen_response_mpu6050 = addr == MPU6050_ADDRESS;
        seen_response_adxl345 = addr == ADXL345_ADDRESS;
        if (!abort_detected)
        {
            failCount = 0;
            switch (status)
            {
            case MPU_6050_POLL:
            case LIS_FAMILY_POLL:
                accel[0] = bufferRx[1] << 8 | bufferRx[0];
                accel[1] = bufferRx[3] << 8 | bufferRx[2];
                accel[2] = bufferRx[5] << 8 | bufferRx[4];
                restart_alarm_id = add_alarm_in_us(500, restart_handler, this, true);
                break;
            case ADXL_POLL:
                // ADXL345 needs to be scaled
                accel[0] = (bufferRx[1] << 8 | bufferRx[0]) * 64;
                accel[1] = (bufferRx[3] << 8 | bufferRx[2]) * 64;
                accel[2] = (bufferRx[5] << 8 | bufferRx[4]) * 64;
                restart_alarm_id = add_alarm_in_us(500, restart_handler, this, true);
                break;
            case LIS3DH_POLL:
                accel[0] = bufferRx[1] << 8 | bufferRx[0];
                accel[1] = bufferRx[3] << 8 | bufferRx[2];
                accel[2] = bufferRx[5] << 8 | bufferRx[4];
                status = LIS3DH_POLL_AUX;
                restart_alarm_id = add_alarm_in_us(100, restart_handler, this, true);
                break;
            case LIS3DH_POLL_AUX:
                lis3dhAdc[0] = bufferRx[1] << 8 | bufferRx[0];
                lis3dhAdc[1] = bufferRx[3] << 8 | bufferRx[2];
                lis3dhAdc[2] = bufferRx[5] << 8 | bufferRx[4];
                restart_alarm_id = add_alarm_in_us(500, restart_handler, this, true);
                status = LIS3DH_POLL;
                break;
            case ACCEL_INIT:
                switch (addr)
                {
                case LIS3DH_ADDRESS:
                    // printf("found lis3dh 1\r\n");
                    if (bufferRxLis1[0] == LIS3DH_ID)
                    {
                        address = addr;
                        type = AccelerometerType::LIS3DH;
                        status = LIS3DH_CTRL1_INIT;
                        bufferTx[0] = LIS3DH_REG_CTRL1;
                        bufferTx[1] = 0b01110111;
                        interface.dmaWriteRead(addr, bufferTx, 2, nullptr, 0);
                    }
                    else if (bufferRxLis1[0] == SC7A20_ID)
                    {
                        address = addr;
                        type = AccelerometerType::SC7A20;
                        status = SC7A20_CTRL1_INIT;
                        bufferTx[0] = LIS3DH_REG_CTRL1;
                        bufferTx[1] = 0b01110111;
                        interface.dmaWriteRead(addr, bufferTx, 2, nullptr, 0);
                    }
                    else if (bufferRxLis1[0] == LIS3DSH_ID)
                    {
                        address = addr;
                        type = AccelerometerType::LIS3DSH;
                        status = LIS3DSH_CTRL1_INIT;
                        bufferTx[0] = LIS3DH_REG_CTRL1;
                        bufferTx[1] = 0b01111111;
                        interface.dmaWriteRead(addr, bufferTx, 2, nullptr, 0);
                    }
                    else
                    {
                        // printf("unrecognised sensor id: %02x\r\n", bufferRx[0]);
                        type = AccelerometerType::None;
                        restart_alarm_id = add_alarm_in_us(200, restart_handler, this, true);
                        return;
                    }
                    break;
                case LIS3DH_ADDRESS_2:
                    if (bufferRxLis2[0] == LIS3DH_ID)
                    {
                        address = addr;
                        type = AccelerometerType::LIS3DH;
                        status = LIS3DH_CTRL1_INIT;
                        bufferTx[0] = LIS3DH_REG_CTRL1;
                        bufferTx[1] = 0b01110111;
                        interface.dmaWriteRead(addr, bufferTx, 2, nullptr, 0);
                    }
                    else if (bufferRxLis2[0] == SC7A20_ID)
                    {
                        address = addr;
                        type = AccelerometerType::SC7A20;
                        status = SC7A20_CTRL1_INIT;
                        bufferTx[0] = LIS3DH_REG_CTRL1;
                        bufferTx[1] = 0b01110111;
                        interface.dmaWriteRead(addr, bufferTx, 2, nullptr, 0);
                    }
                    else if (bufferRxLis2[0] == LIS3DSH_ID)
                    {
                        address = addr;
                        type = AccelerometerType::LIS3DSH;
                        status = LIS3DSH_CTRL1_INIT;
                        bufferTx[0] = LIS3DH_REG_CTRL1;
                        bufferTx[1] = 0b01111111;
                        interface.dmaWriteRead(addr, bufferTx, 2, nullptr, 0);
                    }
                    else
                    {
                        // printf("unrecognised sensor id: %02x\r\n", bufferRx[0]);
                        restart_alarm_id = add_alarm_in_us(200, restart_handler, this, true);
                        return;
                    }
                    break;
                case ADXL345_ADDRESS:
                    // printf("found ADXL sensor at addr: %02x\r\n", addr);
                    // printf("found ADXL sensor id: %02x\r\n", bufferRx[0]);
                    if (bufferRxAdxl[0] != ADXL345_ID)
                    {
                        // printf("unrecognised sensor id: %02x\r\n", bufferRx[0]);
                        type = AccelerometerType::None;
                        restart_alarm_id = add_alarm_in_us(200, restart_handler, this, true);
                        return;
                    }
                    address = addr;
                    type = AccelerometerType::ADXL345;
                    status = ADXL_POWERCTL;
                    bufferTx[0] = ADXL345_POWER_CTL;
                    bufferTx[1] = 0x08;
                    interface.dmaWriteRead(addr, bufferTx, 2, nullptr, 0);
                    break;
                case MPU6050_ADDRESS:
                    // printf("found MPU sensor id: %02x\r\n", bufferRx[0]);
                    if (bufferRxMpu[0] != MPU6050_ID)
                    {
                        if (bufferRxMpu[0] == MPU6050_FAKE_ID)
                        {
                            // printf("It appears you have a fake MPU_6050, we dont support it");
                        }
                        // printf("unrecognised sensor id: %02x\r\n", bufferRx[0]);
                        type = AccelerometerType::None;
                        restart_alarm_id = add_alarm_in_us(200, restart_handler, this, true);
                        return;
                    }
                    address = addr;
                    type = AccelerometerType::MPU6050;
                    status = MPU_6050_ACCEL_CONFIG;
                    bufferTx[0] = MPU6050_REG_ACCEL_CONFIG;
                    bufferTx[1] = 0b11100111;
                    interface.dmaWriteRead(addr, bufferTx, 2, nullptr, 0);
                    break;
                }
                break;
            case ADXL_POWERCTL:
                status = ADXL_DATAFORMAT;
                bufferTx[0] = ADXL345_DATA_FORMAT;
                bufferTx[1] = 0x0B;
                interface.dmaWriteRead(addr, bufferTx, 2, nullptr, 0);
                break;
            case ADXL_DATAFORMAT:
                status = ADXL_POLL;
                restart_alarm_id = add_alarm_in_us(200, restart_handler, this, true);
                break;
            case MPU_6050_ACCEL_CONFIG:
                status = MPU_6050_PWR_MGMT_1_READ;
                bufferTx[0] = MPU6050_REG_PWR_MGMT_1;
                interface.dmaWriteRead(addr, bufferTx, 1, bufferRx, 1);
                break;
            case MPU_6050_PWR_MGMT_1_READ:
                status = MPU_6050_PWR_MGMT_1_WRITE;
                bufferTx[0] = MPU6050_REG_PWR_MGMT_1;
                bufferTx[1] = bufferRx[0] & ~(1 << 6);
                interface.dmaWriteRead(addr, bufferTx, 2, nullptr, 0);
                break;
            case MPU_6050_PWR_MGMT_1_WRITE:
                status = MPU_6050_POLL;
                restart_alarm_id = add_alarm_in_us(200, restart_handler, this, true);
                break;
            case LIS3DH_CTRL1_INIT:
                status = LIS3DH_CTRL4_INIT;
                bufferTx[0] = LIS3DH_REG_CTRL4;
                bufferTx[1] = 0x88;
                interface.dmaWriteRead(addr, bufferTx, 2, nullptr, 0);
                break;
            case LIS3DH_CTRL4_INIT:
                status = LIS3DH_TEMPCFG_INIT;
                bufferTx[0] = LIS3DH_REG_TEMPCFG;
                bufferTx[1] = 0x80;
                interface.dmaWriteRead(addr, bufferTx, 2, nullptr, 0);
                break;
            case LIS3DH_TEMPCFG_INIT:
                // printf("lis3dh ready!\r\n");
                status = LIS3DH_POLL;
                restart_alarm_id = add_alarm_in_us(200, restart_handler, this, true);
                break;
            case LIS3DSH_CTRL1_INIT:
                status = LIS_FAMILY_POLL;
                // printf("lis3dsh ready!\r\n");
                restart_alarm_id = add_alarm_in_us(200, restart_handler, this, true);
                break;
            case SC7A20_CTRL1_INIT:
                status = SC7A20_CTRL4_INIT;
                bufferTx[0] = LIS3DH_REG_CTRL4;
                bufferTx[1] = 0x88;
                interface.dmaWriteRead(addr, bufferTx, 2, nullptr, 0);
                break;
            case SC7A20_CTRL4_INIT:
                status = LIS_FAMILY_POLL;
                // printf("sc7a20 ready!\r\n");
                restart_alarm_id = add_alarm_in_us(200, restart_handler, this, true);
                break;
            }
        }
        // add 200us delay between commands otherwise the extension is overwhelmed
        // If we dont see any sensors, wait a bit before looking again
        if (!abort_detected && status == ACCEL_INIT && seen_response_lis3dh_1 && seen_response_lis3dh_2 && seen_response_adxl345 && seen_response_mpu6050)
        {
            restart_alarm_id = add_alarm_in_us(200, restart_handler, this, true);
        }

        return;
    }
    switch (status)
    {
    case LIS3DH_POLL:
    case LIS_FAMILY_POLL:
        bufferTx[0] = LIS3DH_REG_OUT;
        interface.dmaWriteRead(address, bufferTx, 1, bufferRx, 6);
        break;
    case LIS3DH_POLL_AUX:
        bufferTx[0] = LIS3DH_REG_OUTADC1_L;
        interface.dmaWriteRead(address, bufferTx, 1, bufferTx, 6);
        break;
    case MPU_6050_POLL:
        bufferTx[0] = MPU6050_REG_ACCEL_OUT;
        interface.dmaWriteRead(address, bufferTx, 1, bufferRx, 6);
        break;
    case ADXL_POLL:
        bufferTx[0] = ADXL345_DATAX0;
        interface.dmaWriteRead(address, bufferTx, 1, bufferRx, 6);
        break;
    case ACCEL_INIT:
        if (seen_response_lis3dh_1 && seen_response_lis3dh_2 && seen_response_adxl345 && seen_response_mpu6050)
        {
            seen_response_lis3dh_1 = false;
            seen_response_lis3dh_2 = false;
            seen_response_adxl345 = false;
            seen_response_mpu6050 = false;
            bufferTxLis1[0] = LIS3DH_REG_WHOAMI;
            interface.dmaWriteRead(LIS3DH_ADDRESS, bufferTxLis1, 1, bufferRxLis1, 1);
            bufferTxLis2[0] = LIS3DH_REG_WHOAMI;
            interface.dmaWriteRead(LIS3DH_ADDRESS_2, bufferTxLis2, 1, bufferRxLis2, 1);
            bufferTxAdxl[0] = ADXL345_REG_DEVID;
            interface.dmaWriteRead(ADXL345_ADDRESS, bufferTxAdxl, 1, bufferRxAdxl, 1);
            bufferTxMpu[0] = MPU6050_REG_WHO_AM_I;
            interface.dmaWriteRead(MPU6050_ADDRESS, bufferTxMpu, 1, bufferRxMpu, 1);
        }
        break;
    default:
        printf("unknown status: %d\r\n", status);
        break;
    }
}

void Accelerometer::tick()
{
    interface.tick();
}