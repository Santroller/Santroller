#pragma once

#include "i2c.hpp"
#define MPR121_I2CADDR_DEFAULT 0x5A        ///< default I2C address
#define MPR121_TOUCH_THRESHOLD_DEFAULT 5   ///< default touch threshold value
#define MPR121_RELEASE_THRESHOLD_DEFAULT 1 ///< default relese threshold value
// Electrodes 4 to 11 can be used as GPIO instead, bit 0 of the GPIO registers is electrode 4
#define MPR121_FIRST_GPIO 4
#define MPR121_ELECTRODES 12
typedef enum
{
    MPR121_RESET,
    MPR121_CHECK,
    MPR121_INIT,
    MPR121_POLL_TOUCH,
    MPR121_POLL_GPIO,
    MPR121_WRITE_GPIO,
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
    void process_data(uint8_t addr, bool running, bool timeout, bool abort_detected, bool stop_detected);
    inline bool is_connected()
    {
        return status >= MPR121_POLL_TOUCH;
    }
    // How the pins are used, one bit per GPIO (electrode 4 upwards). Pins used as GPIO are
    // left out of touch sensing. Restarts the chip so the new setup takes effect.
    void configure(uint8_t touchpad_count, uint8_t gpio_inputs, uint8_t gpio_outputs, uint8_t pull_ups, uint8_t pull_downs);
    void set_outputs(uint8_t outputs);
    // Touched electrodes, one bit each
    volatile uint16_t inputs = 0;
    // Levels of the GPIO pins, bit 0 is electrode 4
    volatile uint8_t gpio = 0;

private:
    void start();
    void schedule(uint32_t us);
    void build_init();
    void after_poll();
    I2CMasterInterface interface;
    uint8_t touchpadCount = 0;
    uint8_t gpioInputs = 0;
    uint8_t gpioOutputs = 0;
    uint8_t pullUps = 0;
    uint8_t pullDowns = 0;
    volatile uint8_t outputs = 0;
    volatile bool outputsDirty = false;
    volatile bool restartRequested = false;
    // register / value pairs written in order after a reset
    uint8_t initWrites[64][2];
    uint8_t initCount = 0;
    uint8_t initIndex = 0;
    mpr121_status_e status = MPR121_RESET;
    uint8_t bufferTx[32];
    uint8_t bufferRx[32];
    alarm_id_t restart_alarm_id = 0;
    int failCount = 0;
};
