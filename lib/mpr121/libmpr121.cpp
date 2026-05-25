#include "libmpr121.hpp"
#include "stdio.h"
static int64_t restart_handler(__unused alarm_id_t id, void *user_data)
{
    MPR121 *inst = (MPR121 *)user_data;
    if (inst)
    {
        inst->processData(0, false, false, false, false);
    }
    return 0;
}
void MPR121::tick()
{
    interface.tick();
}

void MPR121::init()
{
}
void MPR121::begin()
{
    interface.dmaInit(MPR121_I2CADDR_DEFAULT, this);
    status = MPR121_INIT_SOFTRESET;
    processData(0, false, false, false, false);
}
void MPR121::end()
{
    cancel_alarm(restart_alarm_id);
    interface.dmaDeinit(MPR121_I2CADDR_DEFAULT);
}
void MPR121::processData(uint8_t addr, bool running, bool timeout, bool abort_detected, bool stop_detected)
{
    cancel_alarm(restart_alarm_id);
    if (timeout || abort_detected)
    {
        failCount++;
        if (failCount > 10)
        {
            status = MPR121_INIT_SOFTRESET;
        }
        restart_alarm_id = add_alarm_in_ms(500, restart_handler, this, true);
        return;
    }
    if (stop_detected)
    {
        failCount = 0;
        switch (status)
        {
        case MPR121_INIT_SOFTRESET:
            status = MPR121_INIT_ECR_CLEAR;
            bufferTx[0] = MPR121_SOFTRESET;
            bufferTx[1] = 0x63;
            interface.dmaWriteRead(MPR121_I2CADDR_DEFAULT, bufferTx, 2, nullptr, 0);
            return;
        case MPR121_INIT_ECR_CLEAR:
            status = MPR121_INIT_CONFIG2_READ;
            bufferTx[0] = MPR121_CONFIG2;
            interface.dmaWriteRead(MPR121_I2CADDR_DEFAULT, bufferTx, 1, bufferRx, 1);
            return;
        case MPR121_INIT_CONFIG2_READ:
            if (bufferRx[0] != 0x24)
            {
                status = MPR121_INIT_SOFTRESET;
                restart_alarm_id = add_alarm_in_ms(500, restart_handler, this, true);
            }
            initTouchpad = 0;
            status = MPR121_INIT_TOUCHTH_N;
            bufferTx[0] = MPR121_TOUCHTH_0;
            bufferTx[1] = MPR121_TOUCH_THRESHOLD_DEFAULT;
            interface.dmaWriteRead(MPR121_I2CADDR_DEFAULT, bufferTx, 1, bufferRx, 1);
            return;
        case MPR121_INIT_TOUCHTH_N:
            status = MPR121_INIT_RELEASETH_N;
            bufferTx[0] = MPR121_RELEASETH_0 + 2 * initTouchpad;
            bufferTx[1] = MPR121_RELEASE_THRESHOLD_DEFAULT;
            initTouchpad++;
            if (initTouchpad == touchpadCount)
            {
                status = MPR121_INIT_MHDR;
                bufferTx[0] = MPR121_MHDR;
                bufferTx[1] = 0x01;
                interface.dmaWriteRead(MPR121_I2CADDR_DEFAULT, bufferTx, 2, nullptr, 0);
                return;
            }
            interface.dmaWriteRead(MPR121_I2CADDR_DEFAULT, bufferTx, 1, bufferRx, 1);
            return;
        case MPR121_INIT_RELEASETH_N:
            status = MPR121_INIT_TOUCHTH_N;
            bufferTx[0] = MPR121_TOUCHTH_0 + 2 * initTouchpad;
            bufferTx[1] = MPR121_TOUCH_THRESHOLD_DEFAULT;
            interface.dmaWriteRead(MPR121_I2CADDR_DEFAULT, bufferTx, 1, bufferRx, 1);
            return;
        case MPR121_INIT_MHDR:
            status = MPR121_INIT_NHDR;
            bufferTx[0] = MPR121_NHDR;
            bufferTx[1] = 0x01;
            interface.dmaWriteRead(MPR121_I2CADDR_DEFAULT, bufferTx, 2, nullptr, 0);
            return;
        case MPR121_INIT_NHDR:
            status = MPR121_INIT_NCLR;
            bufferTx[0] = MPR121_NCLR;
            bufferTx[1] = 0x0E;
            interface.dmaWriteRead(MPR121_I2CADDR_DEFAULT, bufferTx, 2, nullptr, 0);
            return;
        case MPR121_INIT_NCLR:
            status = MPR121_INIT_FDLR;
            bufferTx[0] = MPR121_FDLR;
            bufferTx[1] = 0x00;
            interface.dmaWriteRead(MPR121_I2CADDR_DEFAULT, bufferTx, 2, nullptr, 0);
            return;

        case MPR121_INIT_FDLR:
            status = MPR121_INIT_MHDF;
            bufferTx[0] = MPR121_MHDF;
            bufferTx[1] = 0x01;
            interface.dmaWriteRead(MPR121_I2CADDR_DEFAULT, bufferTx, 2, nullptr, 0);
            return;
        case MPR121_INIT_MHDF:
            status = MPR121_INIT_NCLF;
            bufferTx[0] = MPR121_NCLF;
            bufferTx[1] = 0x01;
            interface.dmaWriteRead(MPR121_I2CADDR_DEFAULT, bufferTx, 2, nullptr, 0);
            return;
        case MPR121_INIT_NCLF:
            status = MPR121_INIT_FDLF;
            bufferTx[0] = MPR121_FDLF;
            bufferTx[1] = 0x00;
            interface.dmaWriteRead(MPR121_I2CADDR_DEFAULT, bufferTx, 2, nullptr, 0);
            return;
        case MPR121_INIT_FDLF:
            status = MPR121_INIT_NHDT;
            bufferTx[0] = MPR121_NHDT;
            bufferTx[1] = 0x00;
            interface.dmaWriteRead(MPR121_I2CADDR_DEFAULT, bufferTx, 2, nullptr, 0);
            return;

        case MPR121_INIT_NHDT:
            status = MPR121_INIT_NCLT;
            bufferTx[0] = MPR121_NCLT;
            bufferTx[1] = 0x00;
            interface.dmaWriteRead(MPR121_I2CADDR_DEFAULT, bufferTx, 2, nullptr, 0);
            return;
        case MPR121_INIT_NCLT:
            status = MPR121_INIT_FDLT;
            bufferTx[0] = MPR121_FDLT;
            bufferTx[1] = 0x00;
            interface.dmaWriteRead(MPR121_I2CADDR_DEFAULT, bufferTx, 2, nullptr, 0);
            return;
        case MPR121_INIT_FDLT:
            status = MPR121_INIT_DEBOUNCE;
            bufferTx[0] = MPR121_DEBOUNCE;
            bufferTx[1] = 0;
            interface.dmaWriteRead(MPR121_I2CADDR_DEFAULT, bufferTx, 2, nullptr, 0);
            return;

        case MPR121_INIT_DEBOUNCE:
            status = MPR121_INIT_CONFIG1;
            bufferTx[0] = MPR121_CONFIG1; // default, 16uA charge current
            bufferTx[1] = 0x10;
            interface.dmaWriteRead(MPR121_I2CADDR_DEFAULT, bufferTx, 2, nullptr, 0);
            return;
        case MPR121_INIT_CONFIG1:
            status = MPR121_INIT_CONFIG2;
            bufferTx[0] = MPR121_CONFIG2;
            bufferTx[1] = 0x20;
            interface.dmaWriteRead(MPR121_I2CADDR_DEFAULT, bufferTx, 2, nullptr, 0);
            return;
        case MPR121_INIT_CONFIG2:
            status = MPR121_INIT_AUTOCONFIG0;
            bufferTx[0] = MPR121_AUTOCONFIG0;
            bufferTx[1] = 0x0B;
            interface.dmaWriteRead(MPR121_I2CADDR_DEFAULT, bufferTx, 2, nullptr, 0);
            return;

        case MPR121_INIT_AUTOCONFIG0:
            status = MPR121_INIT_TARGETLIMIT;
            bufferTx[0] = MPR121_TARGETLIMIT;
            bufferTx[1] = 180; // UPLIMIT * 0.9
            interface.dmaWriteRead(MPR121_I2CADDR_DEFAULT, bufferTx, 2, nullptr, 0);
            return;

        case MPR121_INIT_TARGETLIMIT:
            status = MPR121_INIT_LOWLIMIT;
            bufferTx[0] = MPR121_LOWLIMIT;
            bufferTx[1] = 130; // UPLIMIT * 0.65
            interface.dmaWriteRead(MPR121_I2CADDR_DEFAULT, bufferTx, 2, nullptr, 0);
            return;
        case MPR121_INIT_LOWLIMIT:
            if (enable)
            {
                status = MPR121_INIT_GPIODIR;
                bufferTx[0] = MPR121_GPIODIR;
                bufferTx[1] = ddr;
                interface.dmaWriteRead(MPR121_I2CADDR_DEFAULT, bufferTx, 2, nullptr, 0);
            }
            else
            {
                status = MPR121_INIT_ECR_START;
                bufferTx[0] = MPR121_ECR;
                // 5 bits for baseline tracking & proximity disabled + X amount of electrodes running
                bufferTx[1] = 0b10000000 + touchpadCount;
                interface.dmaWriteRead(MPR121_I2CADDR_DEFAULT, bufferTx, 2, nullptr, 0);
            }
            return;

        case MPR121_INIT_GPIODIR:
            status = MPR121_INIT_GPIOEN;
            bufferTx[0] = MPR121_GPIOEN;
            bufferTx[1] = enable;
            interface.dmaWriteRead(MPR121_I2CADDR_DEFAULT, bufferTx, 2, nullptr, 0);
            return;
        case MPR121_INIT_GPIOEN:
            status = MPR121_INIT_GPIOCTL1;
            bufferTx[0] = MPR121_GPIOCTL1;
            bufferTx[1] = 0xFF;
            interface.dmaWriteRead(MPR121_I2CADDR_DEFAULT, bufferTx, 2, nullptr, 0);
            return;
        case MPR121_INIT_GPIOCTL1:
            status = MPR121_INIT_GPIOCTL2;
            bufferTx[0] = MPR121_GPIOCTL2;
            bufferTx[1] = 0xFF;
            interface.dmaWriteRead(MPR121_I2CADDR_DEFAULT, bufferTx, 2, nullptr, 0);
            return;

        case MPR121_INIT_ECR_START:
            status = MPR121_POLL;
            break;
        case MPR121_POLL:
            inputs = bufferRx[0] << 8 | bufferRx[1];
            break;
        }

        restart_alarm_id = add_alarm_in_us(500, restart_handler, this, true);
        return;
    }
    switch (status)
    {
    case MPR121_POLL:
        bufferTx[0] = MPR121_TOUCHSTATUS_L;
        interface.dmaWriteRead(MPR121_I2CADDR_DEFAULT, bufferTx, 1, bufferRx, 1);
        break;
    case MPR121_INIT_SOFTRESET:
        bufferTx[0] = MPR121_SOFTRESET;
        bufferTx[1] = 0x63;
        interface.dmaWriteRead(MPR121_I2CADDR_DEFAULT, bufferTx, 2, nullptr, 0);
        break;
    default:
        printf("unknown status: %d\r\n", status);
        break;
    }
}