#include "accelerometerlib.hpp"
#include "hardware/sync.h"
#include "stdio.h"
#include <string.h>
#include <math.h>
#include <algorithm>
#include "utils.h"
#include "hardware/gpio.h"
#include "emulation/usb/hid_device.h"
static const uint8_t lis3dh_init = LIS3DH_REG_WHOAMI;
static const uint8_t adxl345_init = ADXL345_REG_DEVID;
static const uint8_t mpu6050_init = MPU6050_REG_WHO_AM_I;

static int64_t __not_in_flash_func(restart_handler)(__unused alarm_id_t id, void *user_data)
{
    Accelerometer *inst = (Accelerometer *)user_data;
    if (inst)
    {
        inst->process_data(0, false, false, false, false);
    }
    return 0;
}

// Fast enough to catch every sample from the 400Hz LIS parts and the 200Hz ADXL345 (the MPU-6050's 1kHz
// is decimated). Each poll costs an alarm and an I2C interrupt, so polling faster just eats core 0
#define ACCEL_POLL_US 2000
// How long to wait before probing again when no accelerometer answered
#define ACCEL_PROBE_RETRY_MS 500
#define ACCEL_1G Accelerometer::ONE_G
// Stop gating after this long, so a chip that reads a different scale, or constant shaking,
// can't freeze the output
#define ACCEL_GATE_MAX_US 250000
// A gap this long between samples (a dropped transfer, or re-probing) starts the filters again
#define ACCEL_FILTER_RESTART_US 100000

Accelerometer::Accelerometer(uint8_t block, uint8_t sda, uint8_t scl, uint32_t clock, float gate_tolerance, uint8_t median_samples, float low_pass_cutoff)
    : interface(block, sda, scl, clock),
      gateTolerance(gate_tolerance > 0 ? gate_tolerance : 0),
      medianSamples(median_samples > 1 ? std::min(median_samples, MEDIAN_MAX) : 0),
      lowPassCutoff(low_pass_cutoff > 0 ? low_pass_cutoff : 0)
{
}

void __not_in_flash_func(Accelerometer::schedule_us)(uint32_t delay_us)
{
    alarm_id_t id = add_alarm_in_us(delay_us, restart_handler, this, true);
    restart_alarm_id = (id > 0) ? id : 0;
}

void Accelerometer::clear()
{
    memset(accel, 0, sizeof(accel));
    memset(gyro, 0, sizeof(gyro));
    memset(lis3dhAdc, 0, sizeof(lis3dhAdc));
    filterReset = true;
}

void __not_in_flash_func(Accelerometer::push_sample)(int32_t x, int32_t y, int32_t z, int32_t gx, int32_t gy, int32_t gz)
{
    uint8_t next = (sampleHead + 1) % SAMPLE_RING_SIZE;
    if (next == sampleTail)
    {
        // tick() fell behind, drop this sample rather than touch the tail it owns
        return;
    }
    Sample &sample = sampleRing[sampleHead];
    sample.accel[0] = x;
    sample.accel[1] = y;
    sample.accel[2] = z;
    sample.gyro[0] = gx;
    sample.gyro[1] = gy;
    sample.gyro[2] = gz;
    sample.time_us = time_us_32();
    __dmb();
    sampleHead = next;
}

void Accelerometer::filter_sample(const Sample &sample)
{
    // the filters are for tilt, which only reads acceleration. Gyro is passed on as it is, even
    // for samples the gate drops, as a strum bar hit is a shake rather than a turn
    memcpy(gyro, sample.gyro, sizeof(gyro));
    float out[3] = {(float)sample.accel[0], (float)sample.accel[1], (float)sample.accel[2]};
    uint32_t dt = sample.time_us - lastSampleUs;
    if (!filterStarted || dt > ACCEL_FILTER_RESTART_US)
    {
        filterStarted = true;
        lastSampleUs = sample.time_us;
        samplePeriod = 0;
        gated = false;
        medianPos = 0;
        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < MEDIAN_MAX; j++)
            {
                medianRing[i][j] = sample.accel[i];
            }
            lowPassIc1[i] = 0;
            lowPassIc2[i] = out[i];
            accel[i] = sample.accel[i];
        }
        return;
    }
    lastSampleUs = sample.time_us;
    // samples are timestamped when the poll that found them finishes, so smooth out that jitter
    samplePeriod = samplePeriod ? samplePeriod + (dt - samplePeriod) * 0.05f : dt;
    if (gateTolerance)
    {
        float mag = sqrtf(out[0] * out[0] + out[1] * out[1] + out[2] * out[2]);
        if (fabsf(mag - ACCEL_1G) > gateTolerance * ACCEL_1G)
        {
            if (!gated)
            {
                gated = true;
                gatedSinceUs = sample.time_us;
            }
            if (sample.time_us - gatedSinceUs < ACCEL_GATE_MAX_US)
            {
                return;
            }
        }
        else
        {
            gated = false;
        }
    }
    if (medianSamples)
    {
        for (int i = 0; i < 3; i++)
        {
            medianRing[i][medianPos] = sample.accel[i];
            int32_t sorted[MEDIAN_MAX];
            memcpy(sorted, medianRing[i], medianSamples * sizeof(int32_t));
            std::nth_element(sorted, sorted + medianSamples / 2, sorted + medianSamples);
            out[i] = sorted[medianSamples / 2];
        }
        medianPos = (medianPos + 1) % medianSamples;
    }
    if (lowPassCutoff)
    {
        // Topology preserving state variable filter (Zavalishin), which keeps its accuracy at cutoffs
        // far below the sample rate, where a float biquad loses it
        if (fabsf(samplePeriod - lowPassPeriod) > lowPassPeriod * 0.01f)
        {
            lowPassPeriod = samplePeriod;
            float w = (float)M_PI * lowPassCutoff * samplePeriod / 1000000.0f;
            float g = tanf(std::min(w, 0.45f * (float)M_PI));
            float k = (float)M_SQRT2;
            lowPassA1 = 1.0f / (1.0f + g * (g + k));
            lowPassA2 = g * lowPassA1;
            lowPassA3 = g * lowPassA2;
        }
        for (int i = 0; i < 3; i++)
        {
            float v3 = out[i] - lowPassIc2[i];
            float v1 = lowPassA1 * lowPassIc1[i] + lowPassA2 * v3;
            float v2 = lowPassIc2[i] + lowPassA2 * lowPassIc1[i] + lowPassA3 * v3;
            lowPassIc1[i] = 2 * v1 - lowPassIc1[i];
            lowPassIc2[i] = 2 * v2 - lowPassIc2[i];
            out[i] = v2;
        }
    }
    for (int i = 0; i < 3; i++)
    {
        accel[i] = lrintf(out[i]);
    }
}

void Accelerometer::begin()
{
    printf("accel begin\r\n");
    m_stopped = false;
    interface.dmaInit(ADXL345_ADDRESS, this);
    interface.dmaInit(ADXL345_ADDRESS_2, this);
    interface.dmaInit(MPU6050_ADDRESS, this);
    interface.dmaInit(MPU6050_ADDRESS_2, this);
    interface.dmaInit(LIS3DH_ADDRESS, this);
    interface.dmaInit(LIS3DH_ADDRESS_2, this);
    probesPending = 0;
    failCount = 0;
    type = AccelerometerType::None;
    clear();
    status = ACCEL_INIT;
    process_data(0, false, false, false, false);
}
void Accelerometer::end()
{
    printf("accel end\r\n");
    // Stop polls scheduling more before cancelling the pending one, or one finishing in between
    // would leave an alarm that fires after this is freed
    m_stopped = true;
    interface.dmaDeinit(ADXL345_ADDRESS);
    interface.dmaDeinit(ADXL345_ADDRESS_2);
    interface.dmaDeinit(MPU6050_ADDRESS);
    interface.dmaDeinit(MPU6050_ADDRESS_2);
    interface.dmaDeinit(LIS3DH_ADDRESS);
    interface.dmaDeinit(LIS3DH_ADDRESS_2);
    cancel_alarm(restart_alarm_id);
    restart_alarm_id = 0;
    status = ACCEL_INIT;
    type = AccelerometerType::None;
    clear();
}
void __not_in_flash_func(Accelerometer::process_data)(uint8_t addr, bool running, bool timeout, bool abort_detected, bool stop_detected)
{
    if (m_stopped)
    {
        return;
    }
    // printf("process_data %02x %d %d %d %d %d\r\n", addr, status, running, timeout, abort_detected, stop_detected);
    
    // If we have started init, ignore i2c data from the other accelerometers
    if (status != ACCEL_INIT && addr && addr != address)
    {
        return;
    }
    lastPoll = to_ms_since_boot(get_absolute_time());
    // printf("%d %d %d %d", status, timeout, abort_detected, stop_detected);
    if (restart_alarm_id)
    {
        cancel_alarm(restart_alarm_id);
        restart_alarm_id = 0;
    }
    if ((timeout || abort_detected || stop_detected) && status == ACCEL_INIT && addr && probesPending)
    {
        probesPending--;
    }
    if (timeout || abort_detected)
    {
        if (status == ACCEL_INIT)
        {
            // Nothing at this address
            probe_round_done();
            return;
        }
        // during high load, there might be the occassional drop, so allow a few failures
        failCount++;
        if (failCount > 10)
        {
            failCount = 0;
            status = ACCEL_INIT;
            type = AccelerometerType::None;
            probesPending = 0;
            clear();
            probe_later();
            return;
        }
        // Polls are retried as they are, a failed init step restarts from probing
        schedule_us(ACCEL_POLL_US);
        return;
    }
    if (stop_detected)
    {
        failCount = 0;
        switch (status)
        {
        // Polls read the data ready flag along with the data, as they run faster than the
        // accelerometer samples, and the filters should only see each sample once
        case MPU_6050_POLL:
            // acceleration, temperature, then gyro
            if (bufferRx[0] & MPU6050_INT_DATA_RDY)
            {
                push_sample((int16_t)(bufferRx[1] << 8 | bufferRx[2]), (int16_t)(bufferRx[3] << 8 | bufferRx[4]), (int16_t)(bufferRx[5] << 8 | bufferRx[6]),
                            (int16_t)(bufferRx[9] << 8 | bufferRx[10]), (int16_t)(bufferRx[11] << 8 | bufferRx[12]), (int16_t)(bufferRx[13] << 8 | bufferRx[14]));
            }
            schedule_us(ACCEL_POLL_US);
            break;
        case LIS_FAMILY_POLL:
            if (bufferRx[0] & LIS3DH_STATUS_ZYXDA)
            {
                push_sample((int16_t)(bufferRx[2] << 8 | bufferRx[1]), (int16_t)(bufferRx[4] << 8 | bufferRx[3]), (int16_t)(bufferRx[6] << 8 | bufferRx[5]));
            }
            schedule_us(ACCEL_POLL_US);
            break;
        case ADXL_POLL:
            // ADXL345 needs to be scaled. The data format register sits between the flags and the data.
            if (bufferRx[0] & ADXL345_INT_SOURCE_DATA_READY)
            {
                push_sample((int16_t)(bufferRx[3] << 8 | bufferRx[2]) * 64, (int16_t)(bufferRx[5] << 8 | bufferRx[4]) * 64, (int16_t)(bufferRx[7] << 8 | bufferRx[6]) * 64);
            }
            schedule_us(ACCEL_POLL_US);
            break;
        case LIS3DH_POLL:
            if (bufferRx[0] & LIS3DH_STATUS_ZYXDA)
            {
                push_sample((int16_t)(bufferRx[2] << 8 | bufferRx[1]), (int16_t)(bufferRx[4] << 8 | bufferRx[3]), (int16_t)(bufferRx[6] << 8 | bufferRx[5]));
            }
            status = LIS3DH_POLL_AUX;
            pollReg = LIS3DH_REG_OUTADC1_L;
            schedule_us(100);
            break;
        case LIS3DH_POLL_AUX:
            lis3dhAdc[0] = bufferRx[1] << 8 | bufferRx[0];
            lis3dhAdc[1] = bufferRx[3] << 8 | bufferRx[2];
            lis3dhAdc[2] = bufferRx[5] << 8 | bufferRx[4];
            status = LIS3DH_POLL;
            pollReg = LIS3DH_REG_STATUS_AI;
            schedule_us(ACCEL_POLL_US);
            break;
        case ACCEL_INIT:
            switch (addr)
            {
            case LIS3DH_ADDRESS:
            case LIS3DH_ADDRESS_2:
            {
                uint8_t idResponse = addr == LIS3DH_ADDRESS ? idResponseLis1 : idResponseLis2;
                if (idResponse == LIS3DH_ID)
                {
                    address = addr;
                    type = AccelerometerType::LIS3DH;
                    status = LIS3DH_CTRL1_INIT;
                    bufferTx[0] = LIS3DH_REG_CTRL1;
                    bufferTx[1] = 0b01110111;
                    interface.dmaWriteRead(addr, bufferTx, 2, nullptr, 0);
                }
                else if (idResponse == SC7A20_ID)
                {
                    address = addr;
                    type = AccelerometerType::SC7A20;
                    status = SC7A20_CTRL1_INIT;
                    bufferTx[0] = LIS3DH_REG_CTRL1;
                    bufferTx[1] = 0b01110111;
                    interface.dmaWriteRead(addr, bufferTx, 2, nullptr, 0);
                }
                else if (idResponse == LIS3DSH_ID)
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
                    // Something else is at this address
                    probe_round_done();
                    return;
                }
                break;
            }
            case ADXL345_ADDRESS:
            case ADXL345_ADDRESS_2:
                if (addr == ADXL345_ADDRESS && idResponseAdxl1 != ADXL345_ID)
                {
                    // Something else is at this address
                    probe_round_done();
                    return;
                }
                if (addr == ADXL345_ADDRESS_2 && idResponseAdxl2 != ADXL345_ID)
                {
                    // Something else is at this address
                    probe_round_done();
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
            case MPU6050_ADDRESS_2:
                if (addr == MPU6050_ADDRESS && (idResponseMpu1 != MPU6050_ID && idResponseMpu1 != MPU6050_ID2 && idResponseMpu1 != MPU6050_ID3))
                {
                    // Something else is at this address
                    probe_round_done();
                    return;
                }
                if (addr == MPU6050_ADDRESS_2 && (idResponseMpu2 != MPU6050_ID && idResponseMpu2 != MPU6050_ID2 && idResponseMpu2 != MPU6050_ID3))
                {
                    // Something else is at this address
                    probe_round_done();
                    return;
                }
                address = addr;
                type = AccelerometerType::MPU6050;
                status = MPU_6050_PWR_MGMT_1_READ;
                bufferTx[0] = MPU6050_REG_PWR_MGMT_1;
                // read it back so the write below only changes sleep and the clock source
                interface.dmaWriteRead(addr, bufferTx, 1, bufferRx, 1);
                break;
            }
            break;
        case ADXL_POWERCTL:
            status = ADXL_DATAFORMAT;
            bufferTx[0] = ADXL345_DATA_FORMAT;
            bufferTx[1] = 0x0B;
            interface.dmaWriteRead(addr, bufferTx, 2, nullptr, 0);
            break;
        case MPU_6050_PWR_MGMT_1_READ:
            status = MPU_6050_PWR_MGMT_1_WRITE;
            bufferTx[0] = MPU6050_REG_PWR_MGMT_1;
            bufferTx[1] = bufferRx[0] & ~(MPU6050_PWR1_SLEEP | MPU6050_PWR1_CLKSEL_MASK);
            bufferTx[1] |= MPU6050_PWR1_CLKSEL_XGYRO;
            interface.dmaWriteRead(addr, bufferTx, 2, nullptr, 0);
            break;
        case MPU_6050_PWR_MGMT_1_WRITE:
            status = MPU_6050_ACCEL_CONFIG;
            bufferTx[0] = MPU6050_REG_ACCEL_CONFIG;
            bufferTx[1] = MPU6050_ACCEL_CONFIG_2G;
            interface.dmaWriteRead(addr, bufferTx, 2, nullptr, 0);
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
        case SC7A20_CTRL1_INIT:
            status = SC7A20_CTRL4_INIT;
            bufferTx[0] = LIS3DH_REG_CTRL4;
            bufferTx[1] = 0x88;
            interface.dmaWriteRead(addr, bufferTx, 2, nullptr, 0);
            break;
        case MPU_6050_ACCEL_CONFIG:
            status = MPU_6050_GYRO_CONFIG;
            bufferTx[0] = MPU6050_REG_GYRO_CONFIG;
            bufferTx[1] = MPU6050_GYRO_CONFIG_2000DPS;
            interface.dmaWriteRead(addr, bufferTx, 2, nullptr, 0);
            break;
        case MPU_6050_GYRO_CONFIG:
            // a known 1kHz sample rate, so the data ready flag tracks real accelerometer samples
            status = MPU_6050_CONFIG;
            bufferTx[0] = MPU6050_REG_CONFIG;
            bufferTx[1] = MPU6050_CONFIG_DLPF_184HZ;
            interface.dmaWriteRead(addr, bufferTx, 2, nullptr, 0);
            break;
        case MPU_6050_CONFIG:
            // the data ready flag in INT_STATUS is only raised while its interrupt is enabled
            status = MPU_6050_INT_ENABLE;
            bufferTx[0] = MPU6050_REG_INT_ENABLE;
            bufferTx[1] = MPU6050_INT_DATA_RDY;
            interface.dmaWriteRead(addr, bufferTx, 2, nullptr, 0);
            break;
        case MPU_6050_INT_ENABLE:
            status = MPU_6050_POLL;
            pollReg = MPU6050_REG_INT_STATUS;
            schedule_us(ACCEL_POLL_US);
            break;
        case ADXL_DATAFORMAT:
            // 200Hz is as fast as the ADXL345 should go on a 100kHz bus
            status = ADXL_BW_RATE;
            bufferTx[0] = ADXL345_BW_RATE;
            bufferTx[1] = ADXL345_BW_RATE_200HZ;
            interface.dmaWriteRead(addr, bufferTx, 2, nullptr, 0);
            break;
        case ADXL_BW_RATE:
            status = ADXL_POLL;
            pollReg = ADXL345_INT_SOURCE;
            schedule_us(ACCEL_POLL_US);
            break;
        case LIS3DH_TEMPCFG_INIT:
            status = LIS3DH_POLL;
            pollReg = LIS3DH_REG_STATUS_AI;
            schedule_us(ACCEL_POLL_US);
            break;
        case LIS3DSH_CTRL1_INIT:
            status = LIS_FAMILY_POLL;
            pollReg = LIS3DH_REG_STATUS_AI;
            schedule_us(ACCEL_POLL_US);
            break;
        case SC7A20_CTRL4_INIT:
            status = LIS_FAMILY_POLL;
            pollReg = LIS3DH_REG_STATUS_AI;
            schedule_us(ACCEL_POLL_US);
            break;
        }
        return;
    }
    switch (status)
    {
    case LIS3DH_POLL_AUX:
        interface.dmaWriteRead(address, &pollReg, 1, bufferRx, 6);
        break;
    case LIS3DH_POLL:
    case LIS_FAMILY_POLL:
        // status, then the outputs
        interface.dmaWriteRead(address, &pollReg, 1, bufferRx, 7);
        break;
    case MPU_6050_POLL:
        // status, acceleration, temperature and gyro
        interface.dmaWriteRead(address, &pollReg, 1, bufferRx, 15);
        break;
    case ADXL_POLL:
        // interrupt source, data format, then the outputs
        interface.dmaWriteRead(address, &pollReg, 1, bufferRx, 8);
        break;
    case ACCEL_INIT:
        // Probe every address, the answers (or NAKs) count probesPending back down
        if (probesPending)
        {
            break;
        }
        probesPending += interface.dmaWriteRead(LIS3DH_ADDRESS, &lis3dh_init, 1, &idResponseLis1, 1);
        probesPending += interface.dmaWriteRead(LIS3DH_ADDRESS_2, &lis3dh_init, 1, &idResponseLis2, 1);
        probesPending += interface.dmaWriteRead(ADXL345_ADDRESS, &adxl345_init, 1, &idResponseAdxl1, 1);
        probesPending += interface.dmaWriteRead(ADXL345_ADDRESS_2, &adxl345_init, 1, &idResponseAdxl2, 1);
        probesPending += interface.dmaWriteRead(MPU6050_ADDRESS, &mpu6050_init, 1, &idResponseMpu1, 1);
        probesPending += interface.dmaWriteRead(MPU6050_ADDRESS_2, &mpu6050_init, 1, &idResponseMpu2, 1);
        if (!probesPending)
        {
            probe_later();
        }
        break;
    default:
        status = ACCEL_INIT;
        type = AccelerometerType::None;
        // probes left over from before init started were dropped, so they won't count down
        probesPending = 0;
        schedule_us(ACCEL_POLL_US);
        break;
    }
}

void Accelerometer::probe_later()
{
    alarm_id_t id = add_alarm_in_ms(ACCEL_PROBE_RETRY_MS, restart_handler, this, true);
    restart_alarm_id = (id > 0) ? id : 0;
}

void Accelerometer::probe_round_done()
{
    // Once every address has answered without finding an accelerometer, look again in a bit
    if (!probesPending && status == ACCEL_INIT)
    {
        probe_later();
    }
}

void Accelerometer::tick()
{
    interface.tick();
    if (filterReset)
    {
        filterReset = false;
        filterStarted = false;
        sampleTail = sampleHead;
    }
    while (sampleTail != sampleHead)
    {
        __dmb();
        filter_sample(sampleRing[sampleTail]);
        sampleTail = (sampleTail + 1) % SAMPLE_RING_SIZE;
    }
    if (lastPoll && !restart_alarm_id && to_ms_since_boot(get_absolute_time()) - lastPoll > 1000)
    {
        // Masked so the I2C interrupt can't schedule a poll between process_data cancelling the old
        // alarm and storing the new one, which would leave an alarm that end() can't cancel
        uint32_t irq_status = save_and_disable_interrupts();
        // A transfer never came back, start again from probing
        if (status == ACCEL_INIT || (status != LIS3DH_POLL && status != LIS_FAMILY_POLL && status != LIS3DH_POLL_AUX && status != MPU_6050_POLL && status != ADXL_POLL))
        {
            status = ACCEL_INIT;
            type = AccelerometerType::None;
            probesPending = 0;
        }
        process_data(0, false, false, false, false);
        restore_interrupts(irq_status);
    }
}