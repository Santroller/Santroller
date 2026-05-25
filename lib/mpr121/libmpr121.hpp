#pragma once

#include "i2c.hpp"
#define MPR121_I2CADDR_DEFAULT 0x5A        ///< default I2C address
#define MPR121_TOUCH_THRESHOLD_DEFAULT 5   ///< default touch threshold value
#define MPR121_RELEASE_THRESHOLD_DEFAULT 1 ///< default relese threshold value
typedef enum
{
    MPR121_INIT_SOFTRESET,
    MPR121_INIT_ECR_CLEAR,
    MPR121_INIT_CONFIG2_READ,
    MPR121_INIT_TOUCHTH_N,
    MPR121_INIT_RELEASETH_N,
    MPR121_INIT_MHDR,
    MPR121_INIT_NHDR,
    MPR121_INIT_NCLR,
    MPR121_INIT_FDLR,

    MPR121_INIT_MHDF,
    MPR121_INIT_NHDF,
    MPR121_INIT_NCLF,
    MPR121_INIT_FDLF,

    MPR121_INIT_NHDT,
    MPR121_INIT_NCLT,
    MPR121_INIT_FDLT,

    MPR121_INIT_DEBOUNCE,
    MPR121_INIT_CONFIG1,
    MPR121_INIT_CONFIG2,

    MPR121_INIT_AUTOCONFIG0,

    MPR121_INIT_UPLIMIT,
    MPR121_INIT_TARGETLIMIT,
    MPR121_INIT_LOWLIMIT,

    MPR121_INIT_GPIODIR,
    MPR121_INIT_GPIOEN,
    MPR121_INIT_GPIOCTL1,
    MPR121_INIT_GPIOCTL2,
    MPR121_INIT_ECR_START,
    MPR121_POLL

} mpr121_status_e;
enum
{
    MPR121_TOUCHSTATUS_L = 0x00,
    MPR121_TOUCHSTATUS_H = 0x01,
    MPR121_FILTDATA_0L = 0x04,
    MPR121_FILTDATA_0H = 0x05,
    MPR121_BASELINE_0 = 0x1E,
    MPR121_MHDR = 0x2B,
    MPR121_NHDR = 0x2C,
    MPR121_NCLR = 0x2D,
    MPR121_FDLR = 0x2E,
    MPR121_MHDF = 0x2F,
    MPR121_NHDF = 0x30,
    MPR121_NCLF = 0x31,
    MPR121_FDLF = 0x32,
    MPR121_NHDT = 0x33,
    MPR121_NCLT = 0x34,
    MPR121_FDLT = 0x35,

    MPR121_TOUCHTH_0 = 0x41,
    MPR121_RELEASETH_0 = 0x42,
    MPR121_DEBOUNCE = 0x5B,
    MPR121_CONFIG1 = 0x5C,
    MPR121_CONFIG2 = 0x5D,
    MPR121_CHARGECURR_0 = 0x5F,
    MPR121_CHARGETIME_1 = 0x6C,
    MPR121_ECR = 0x5E,
    MPR121_AUTOCONFIG0 = 0x7B,
    MPR121_AUTOCONFIG1 = 0x7C,
    MPR121_UPLIMIT = 0x7D,
    MPR121_LOWLIMIT = 0x7E,
    MPR121_TARGETLIMIT = 0x7F,

    MPR121_GPIOCTL1 = 0x73,
    MPR121_GPIOCTL2 = 0x74,
    MPR121_GPIODATA = 0x75,
    MPR121_GPIODIR = 0x76,
    MPR121_GPIOEN = 0x77,
    MPR121_GPIOSET = 0x78,
    MPR121_GPIOCLR = 0x79,
    MPR121_GPIOTOGGLE = 0x7A,

    MPR121_SOFTRESET = 0x80,

    // https://files.seeedstudio.com/wiki/Grove-12_Key_Capacitive_I2C_Touch_Sensor_V2-MPR121/res/AN3894.pdf
    MPR121_PWM0 = 0x81,
    MPR121_PWM1 = 0x82,
    MPR121_PWM2 = 0x83,
    MPR121_PWM3 = 0x84,
};
class MPR121 : public I2CDMAInterface
{
public:
    MPR121(uint8_t block, uint8_t sda, uint8_t scl, uint32_t clock)
        : interface(block, sda, scl, clock) {};
    void tick();
    void begin();
    void end();
    void processData(uint8_t addr, bool running, bool timeout, bool abort_detected, bool stop_detected);
    inline bool is_connected()
    {
        return status < MPR121_INIT_TOUCHTH_N;
    }
    uint16_t inputs;

private:
    void init();
    I2CMasterInterface interface;
    int initTouchpad;
    int touchpadCount;
    int ddr;    // pin data direction register
    int enable; // pin enable
    mpr121_status_e status = MPR121_INIT_SOFTRESET;
    uint8_t bufferTx[32];
    uint8_t bufferRx[32];
    alarm_id_t restart_alarm_id;
    int failCount = 0;
};
