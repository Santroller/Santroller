// Support for LIS3DH, MPU-6050 and ADXL345 accelerometers
#pragma once
#include "i2c.hpp"
#define ADXL345_ADDRESS 0x53
#define ADXL345_ADDRESS_2 0x1D
#define ADXL345_BW_RATE 0x2C
#define ADXL345_BW_RATE_200HZ 0x0B
#define ADXL345_POWER_CTL 0x2D
#define ADXL345_INT_SOURCE 0x30
#define ADXL345_INT_SOURCE_DATA_READY 0x80
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
#define LIS3DH_REG_STATUS_AI 0x27 | 0x80 // Status followed by the outputs, with auto increment
#define LIS3DH_STATUS_ZYXDA 0x08
#define LIS3DH_REG_STATUS_AUX 0x07
#define LIS3DH_REG_OUT 0x28 | 0x80 // Set auto increment
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
#define MPU6050_ADDRESS_2 0x69
#define MPU6050_REG_ACCEL_OUT 0x3B
#define MPU6050_REG_CONFIG 0x1A
#define MPU6050_REG_INT_ENABLE 0x38
#define MPU6050_REG_INT_STATUS 0x3A
#define MPU6050_INT_DATA_RDY 0x01
// DLPF_CFG 1: 1kHz sample rate, 184Hz accelerometer bandwidth
#define MPU6050_CONFIG_DLPF_184HZ 0x01
#define MPU6050_REG_WHO_AM_I 0x75
#define MPU6050_REG_ACCEL_CONFIG 0x1C
#define MPU6050_REG_PWR_MGMT_1 0x6B
#define MPU6050_ID 0x68
#define MPU6050_ID2 0x98 // Common register-compatible clone
#define MPU6050_ID3 0x70 // Register-compatible MPU-6500/clone ID
#define MPU6050_PWR1_SLEEP (1U << 6)
#define MPU6050_PWR1_CLKSEL_MASK 0x07
#define MPU6050_PWR1_CLKSEL_XGYRO 0x01
#define MPU6050_ACCEL_CONFIG_2G 0x00
enum AccelerometerType
{
    None,
    ADXL345,
    LIS3DH,
    MPU6050,
    SC7A20,
    LIS3DSH
};
typedef enum
{
    ACCEL_INIT,
    SC7A20_CTRL1_INIT,
    SC7A20_CTRL4_INIT,
    LIS3DH_CTRL1_INIT,
    LIS3DH_CTRL4_INIT,
    LIS3DH_TEMPCFG_INIT,
    LIS3DSH_CTRL1_INIT,
    ADXL_POWERCTL,
    ADXL_DATAFORMAT,
    ADXL_BW_RATE,
    MPU_6050_ACCEL_CONFIG,
    MPU_6050_CONFIG,
    MPU_6050_INT_ENABLE,
    MPU_6050_PWR_MGMT_1_READ,
    MPU_6050_PWR_MGMT_1_WRITE,
    LIS_FAMILY_POLL,
    LIS3DH_POLL,
    LIS3DH_POLL_AUX,
    ADXL_POLL,
    MPU_6050_POLL
} accel_status_e;
class Accelerometer : public I2CDMAInterface
{
public:
    // Filters for each new sample, each off when 0. gate_tolerance (in g) drops samples whose total
    // acceleration is that far from gravity, median_samples takes the median of that many samples,
    // and low_pass_cutoff is a 2nd order Butterworth low pass in Hz.
    Accelerometer(uint8_t block, uint8_t sda, uint8_t scl, uint32_t clock, float gate_tolerance = 0, uint8_t median_samples = 0, float low_pass_cutoff = 0);
    void tick();
    void begin();
    void end();
    inline bool is_connected()
    {
        return status != ACCEL_INIT;
    }
    uint16_t lis3dhAdc[3] = {};
    // The filtered acceleration, updated by tick()
    int32_t accel[3] = {};
    void process_data(uint8_t addr, bool running, bool timeout, bool abort_detected, bool stop_detected);

    static constexpr uint8_t MEDIAN_MAX = 9;

private:
    struct Sample
    {
        int32_t accel[3];
        uint32_t time_us;
    };
    // New samples go through a ring from the I2C interrupt to tick(), so the filters run outside
    // the interrupt but still see every sample
    static constexpr uint8_t SAMPLE_RING_SIZE = 16;
    void push_sample(int32_t x, int32_t y, int32_t z);
    void filter_sample(const Sample &sample);
    void schedule_us(uint32_t delay_us);
    void probe_later();
    void probe_round_done();
    void clear();
    I2CMasterInterface interface;
    AccelerometerType type = AccelerometerType::None;
    uint8_t address = 0;
    int currentPoll = 0;
    int failCount = 0;
    alarm_id_t restart_alarm_id = 0;
    // Set by end(), so a poll still in flight from an alarm or the I2C interrupt doesn't schedule another
    volatile bool m_stopped = false;
    accel_status_e status = ACCEL_INIT;
    uint8_t bufferTx[32];
    uint8_t bufferRx[32];
    uint8_t idResponseLis1 = 0;
    uint8_t idResponseLis2 = 0;
    uint8_t idResponseAdxl1 = 0;
    uint8_t idResponseAdxl2 = 0;
    uint8_t idResponseMpu1 = 0;
    uint8_t idResponseMpu2 = 0;
    uint8_t pollReg = 0;
    uint32_t lastPoll = 0;
    // Probes sent to each possible address that haven't answered yet
    uint8_t probesPending = 0;

    Sample sampleRing[SAMPLE_RING_SIZE];
    volatile uint8_t sampleHead = 0;
    volatile uint8_t sampleTail = 0;
    // Set by clear(), so tick() drops queued samples and starts the filters again
    volatile bool filterReset = true;

    float gateTolerance;
    uint8_t medianSamples;
    float lowPassCutoff;
    bool filterStarted = false;
    uint32_t lastSampleUs = 0;
    // Smoothed time between samples, as the accelerometer's clock is only roughly its nominal rate
    float samplePeriod = 0;
    uint32_t gatedSinceUs = 0;
    bool gated = false;
    int32_t medianRing[3][MEDIAN_MAX];
    uint8_t medianPos = 0;
    // 2nd order state variable filter state, per axis, and its coefficients for the current period
    float lowPassIc1[3];
    float lowPassIc2[3];
    float lowPassPeriod = 0;
    float lowPassA1 = 0;
    float lowPassA2 = 0;
    float lowPassA3 = 0;
};
