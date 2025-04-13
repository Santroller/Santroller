#include "devices/mpr121.hpp"

#include "interfaces/core.hpp"

void MPR121Device::tick(san_base_t* data) {
    if (!mFound) {
        init();
        return;
    }
    uint16_t raw;
    if (!mInterface->readFromPointerRepeatedStart(MPR121_I2CADDR_DEFAULT, MPR121_TOUCHSTATUS_L, sizeof(raw), (uint8_t*)&raw)) {
        mFound = false;
    } else {
        data->extra.mpr121Inputs = raw;
    }
}

void MPR121Device::init() {
    mInterface->writeSingleToPointer(MPR121_I2CADDR_DEFAULT, MPR121_SOFTRESET, 0x63);
    mInterface->writeSingleToPointer(MPR121_I2CADDR_DEFAULT, MPR121_ECR, 0x0);
    uint8_t data;
    mInterface->readFromPointerRepeatedStart(MPR121_I2CADDR_DEFAULT, MPR121_CONFIG2, 1, &data);
    if (data != 0x24) {
        mFound = false;
        return;
    }
    mFound = true;
    for (uint8_t i = 0; i < touchpadCount; i++) {
        mInterface->writeSingleToPointer(MPR121_I2CADDR_DEFAULT, MPR121_TOUCHTH_0 + 2 * i, MPR121_TOUCH_THRESHOLD_DEFAULT);
        mInterface->writeSingleToPointer(MPR121_I2CADDR_DEFAULT, MPR121_RELEASETH_0 + 2 * i, MPR121_RELEASE_THRESHOLD_DEFAULT);
    }
    mInterface->writeSingleToPointer(MPR121_I2CADDR_DEFAULT, MPR121_MHDR, 0x01);
    mInterface->writeSingleToPointer(MPR121_I2CADDR_DEFAULT, MPR121_NHDR, 0x01);
    mInterface->writeSingleToPointer(MPR121_I2CADDR_DEFAULT, MPR121_NCLR, 0x0E);
    mInterface->writeSingleToPointer(MPR121_I2CADDR_DEFAULT, MPR121_FDLR, 0x00);

    mInterface->writeSingleToPointer(MPR121_I2CADDR_DEFAULT, MPR121_MHDF, 0x01);
    mInterface->writeSingleToPointer(MPR121_I2CADDR_DEFAULT, MPR121_NHDF, 0x05);
    mInterface->writeSingleToPointer(MPR121_I2CADDR_DEFAULT, MPR121_NCLF, 0x01);
    mInterface->writeSingleToPointer(MPR121_I2CADDR_DEFAULT, MPR121_FDLF, 0x00);

    mInterface->writeSingleToPointer(MPR121_I2CADDR_DEFAULT, MPR121_NHDT, 0x00);
    mInterface->writeSingleToPointer(MPR121_I2CADDR_DEFAULT, MPR121_NCLT, 0x00);
    mInterface->writeSingleToPointer(MPR121_I2CADDR_DEFAULT, MPR121_FDLT, 0x00);

    mInterface->writeSingleToPointer(MPR121_I2CADDR_DEFAULT, MPR121_DEBOUNCE, 0);
    mInterface->writeSingleToPointer(MPR121_I2CADDR_DEFAULT, MPR121_CONFIG1, 0x10);  // default, 16uA charge current
    mInterface->writeSingleToPointer(MPR121_I2CADDR_DEFAULT, MPR121_CONFIG2, 0x20);

    mInterface->writeSingleToPointer(MPR121_I2CADDR_DEFAULT, MPR121_AUTOCONFIG0, 0x0B);

    // correct values for Vdd = 3.3V
    mInterface->writeSingleToPointer(MPR121_I2CADDR_DEFAULT, MPR121_UPLIMIT, 200);      // ((Vdd - 0.7)/Vdd) * 256
    mInterface->writeSingleToPointer(MPR121_I2CADDR_DEFAULT, MPR121_TARGETLIMIT, 180);  // UPLIMIT * 0.9
    mInterface->writeSingleToPointer(MPR121_I2CADDR_DEFAULT, MPR121_LOWLIMIT, 130);     // UPLIMIT * 0.65

    if (enable) {
        mInterface->writeSingleToPointer(MPR121_I2CADDR_DEFAULT, MPR121_GPIODIR, ddr);
        mInterface->writeSingleToPointer(MPR121_I2CADDR_DEFAULT, MPR121_GPIOEN, enable);
        // Set GPIO to LED driver on outputs and pullup on inputs.
        mInterface->writeSingleToPointer(MPR121_I2CADDR_DEFAULT, MPR121_GPIOCTL1, 0xFF);
        mInterface->writeSingleToPointer(MPR121_I2CADDR_DEFAULT, MPR121_GPIOCTL2, 0xFF);
    }
    // enable electrodes and start MPR121
    uint8_t ECR_SETTING =
        0b10000000 + touchpadCount;
    mInterface->writeSingleToPointer(MPR121_I2CADDR_DEFAULT, MPR121_ECR, 0b10000000 + touchpadCount);  // 5 bits for baseline tracking & proximity disabled + X
                                                                                                       // amount of electrodes running (12)
}