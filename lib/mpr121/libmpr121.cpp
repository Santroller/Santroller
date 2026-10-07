#include "libmpr121.hpp"
#include "stdio.h"
static int64_t restart_handler(__unused alarm_id_t id, void *user_data)
{
    MPR121 *inst = (MPR121 *)user_data;
    if (inst)
    {
        inst->process_data(0, false, false, false, false);
    }
    return 0;
}
void MPR121::tick()
{
    interface.tick();
}

void MPR121::begin()
{
    interface.dmaInit(MPR121_I2CADDR_DEFAULT, this);
    start();
}
void MPR121::end()
{
    cancel_alarm(restart_alarm_id);
    interface.dmaDeinit(MPR121_I2CADDR_DEFAULT);
}

void MPR121::configure(uint8_t touchpad_count, uint8_t gpio_inputs, uint8_t gpio_outputs, uint8_t pull_ups, uint8_t pull_downs)
{
    // An electrode can't be sensing touch and be a GPIO at the same time, and touch sensing
    // always runs from electrode 0 up, so stop before the first GPIO
    uint8_t gpio = gpio_inputs | gpio_outputs;
    for (uint8_t i = 0; i < 8; i++)
    {
        if (gpio & (1 << i) && touchpad_count > MPR121_FIRST_GPIO + i)
        {
            touchpad_count = MPR121_FIRST_GPIO + i;
            break;
        }
    }
    if (touchpad_count > MPR121_ELECTRODES)
    {
        touchpad_count = MPR121_ELECTRODES;
    }
    if (touchpadCount == touchpad_count && gpioInputs == gpio_inputs && gpioOutputs == gpio_outputs &&
        pullUps == pull_ups && pullDowns == pull_downs)
    {
        return;
    }
    touchpadCount = touchpad_count;
    gpioInputs = gpio_inputs;
    gpioOutputs = gpio_outputs;
    pullUps = pull_ups;
    pullDowns = pull_downs;
    restartRequested = true;
}

void MPR121::set_outputs(uint8_t new_outputs)
{
    if (outputs == new_outputs)
    {
        return;
    }
    outputs = new_outputs;
    outputsDirty = true;
}

void MPR121::build_init()
{
    initCount = 0;
    auto add = [this](uint8_t reg, uint8_t value)
    {
        initWrites[initCount][0] = reg;
        initWrites[initCount][1] = value;
        initCount++;
    };
    for (uint8_t i = 0; i < MPR121_ELECTRODES; i++)
    {
        add(MPR121_TOUCHTH_0 + 2 * i, MPR121_TOUCH_THRESHOLD_DEFAULT);
        add(MPR121_RELEASETH_0 + 2 * i, MPR121_RELEASE_THRESHOLD_DEFAULT);
    }
    add(MPR121_MHDR, 0x01);
    add(MPR121_NHDR, 0x01);
    add(MPR121_NCLR, 0x0E);
    add(MPR121_FDLR, 0x00);
    add(MPR121_MHDF, 0x01);
    add(MPR121_NHDF, 0x05);
    add(MPR121_NCLF, 0x01);
    add(MPR121_FDLF, 0x00);
    add(MPR121_NHDT, 0x00);
    add(MPR121_NCLT, 0x00);
    add(MPR121_FDLT, 0x00);
    add(MPR121_DEBOUNCE, 0);
    add(MPR121_CONFIG1, 0x10); // default, 16uA charge current
    add(MPR121_CONFIG2, 0x20);
    add(MPR121_AUTOCONFIG0, 0x0B);
    add(MPR121_UPLIMIT, 200);
    add(MPR121_TARGETLIMIT, 180); // UPLIMIT * 0.9
    add(MPR121_LOWLIMIT, 130);    // UPLIMIT * 0.65
    uint8_t enabled = gpioInputs | gpioOutputs;
    if (enabled)
    {
        // CTL0 CTL1 DIR: 0 0 0 input, 1 0 0 input with pull down, 1 1 0 input with pull up,
        // 0 0 1 push-pull output
        add(MPR121_GPIOCTL1, (pullUps | pullDowns) & gpioInputs);
        add(MPR121_GPIOCTL2, pullUps & gpioInputs);
        add(MPR121_GPIODIR, gpioOutputs);
        add(MPR121_GPIODATA, outputs);
        add(MPR121_GPIOEN, enabled);
    }
    // baseline tracking enabled, proximity disabled, and the electrodes used for touch
    add(MPR121_ECR, 0b10000000 + touchpadCount);
}

void MPR121::start()
{
    cancel_alarm(restart_alarm_id);
    restartRequested = false;
    status = MPR121_RESET;
    bufferTx[0] = MPR121_SOFTRESET;
    bufferTx[1] = 0x63;
    interface.dmaWriteRead(MPR121_I2CADDR_DEFAULT, bufferTx, 2, nullptr, 0);
}

void MPR121::schedule(uint32_t us)
{
    restart_alarm_id = add_alarm_in_us(us, restart_handler, this, true);
}

void MPR121::after_poll()
{
    if (restartRequested)
    {
        start();
        return;
    }
    if (outputsDirty && gpioOutputs)
    {
        outputsDirty = false;
        status = MPR121_WRITE_GPIO;
        bufferTx[0] = MPR121_GPIODATA;
        bufferTx[1] = outputs;
        interface.dmaWriteRead(MPR121_I2CADDR_DEFAULT, bufferTx, 2, nullptr, 0);
        return;
    }
    status = MPR121_POLL_TOUCH;
    schedule(500);
}

void MPR121::process_data(uint8_t addr, bool running, bool timeout, bool abort_detected, bool stop_detected)
{
    cancel_alarm(restart_alarm_id);
    if (timeout || abort_detected)
    {
        // start from scratch once it shows up again
        failCount++;
        status = MPR121_RESET;
        restart_alarm_id = add_alarm_in_ms(500, restart_handler, this, true);
        return;
    }
    if (!stop_detected)
    {
        // a scheduled step
        switch (status)
        {
        case MPR121_POLL_TOUCH:
            bufferTx[0] = MPR121_TOUCHSTATUS_L;
            interface.dmaWriteRead(MPR121_I2CADDR_DEFAULT, bufferTx, 1, bufferRx, 2);
            break;
        default:
            start();
            break;
        }
        return;
    }
    failCount = 0;
    switch (status)
    {
    case MPR121_RESET:
        status = MPR121_CHECK;
        bufferTx[0] = MPR121_CONFIG2;
        interface.dmaWriteRead(MPR121_I2CADDR_DEFAULT, bufferTx, 1, bufferRx, 1);
        return;
    case MPR121_CHECK:
        // CONFIG2 reads 0x24 after a reset
        if (bufferRx[0] != 0x24)
        {
            status = MPR121_RESET;
            schedule(500000);
            return;
        }
        build_init();
        initIndex = 0;
        status = MPR121_INIT;
        // fall through to send the first write
    case MPR121_INIT:
        if (initIndex < initCount)
        {
            bufferTx[0] = initWrites[initIndex][0];
            bufferTx[1] = initWrites[initIndex][1];
            initIndex++;
            interface.dmaWriteRead(MPR121_I2CADDR_DEFAULT, bufferTx, 2, nullptr, 0);
            return;
        }
        outputsDirty = false;
        status = MPR121_POLL_TOUCH;
        schedule(500);
        return;
    case MPR121_POLL_TOUCH:
        inputs = (bufferRx[0] | bufferRx[1] << 8) & 0x0FFF;
        if (gpioInputs)
        {
            status = MPR121_POLL_GPIO;
            bufferTx[0] = MPR121_GPIODATA;
            interface.dmaWriteRead(MPR121_I2CADDR_DEFAULT, bufferTx, 1, bufferRx, 1);
            return;
        }
        after_poll();
        return;
    case MPR121_POLL_GPIO:
        gpio = bufferRx[0];
        after_poll();
        return;
    case MPR121_WRITE_GPIO:
        status = MPR121_POLL_TOUCH;
        schedule(500);
        return;
    }
}
