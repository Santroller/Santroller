#include "mpr121.h"
#include "stdio.h"

#include "config.h"
#include "io.h"
#ifdef INPUT_MPR121
bool mpr121_init = false;
bool init_mpr121() {
    twi_writeSingleToPointer(MPR121_TWI_PORT, MPR121_I2CADDR_DEFAULT, MPR121_SOFTRESET, 0x63);
    twi_writeSingleToPointer(MPR121_TWI_PORT, MPR121_I2CADDR_DEFAULT, MPR121_ECR, 0x0);
    uint8_t data;
    twi_readFromPointerRepeatedStart(MPR121_TWI_PORT, MPR121_I2CADDR_DEFAULT, MPR121_CONFIG2, 1, &data);
    if (data != 0x24) {
        return false;
    }
    mpr121_init = true;
    for (uint8_t i = 0; i < 12; i++) {
        twi_writeSingleToPointer(MPR121_TWI_PORT, MPR121_I2CADDR_DEFAULT, MPR121_TOUCHTH_0 + 2 * i, MPR121_TOUCH_THRESHOLD_DEFAULT);
        twi_writeSingleToPointer(MPR121_TWI_PORT, MPR121_I2CADDR_DEFAULT, MPR121_RELEASETH_0 + 2 * i, MPR121_RELEASE_THRESHOLD_DEFAULT);
    }
    twi_writeSingleToPointer(MPR121_TWI_PORT, MPR121_I2CADDR_DEFAULT, MPR121_MHDR, 0x01);
    twi_writeSingleToPointer(MPR121_TWI_PORT, MPR121_I2CADDR_DEFAULT, MPR121_NHDR, 0x01);
    twi_writeSingleToPointer(MPR121_TWI_PORT, MPR121_I2CADDR_DEFAULT, MPR121_NCLR, 0x0E);
    twi_writeSingleToPointer(MPR121_TWI_PORT, MPR121_I2CADDR_DEFAULT, MPR121_FDLR, 0x00);

    twi_writeSingleToPointer(MPR121_TWI_PORT, MPR121_I2CADDR_DEFAULT, MPR121_MHDF, 0x01);
    twi_writeSingleToPointer(MPR121_TWI_PORT, MPR121_I2CADDR_DEFAULT, MPR121_NHDF, 0x05);
    twi_writeSingleToPointer(MPR121_TWI_PORT, MPR121_I2CADDR_DEFAULT, MPR121_NCLF, 0x01);
    twi_writeSingleToPointer(MPR121_TWI_PORT, MPR121_I2CADDR_DEFAULT, MPR121_FDLF, 0x00);

    twi_writeSingleToPointer(MPR121_TWI_PORT, MPR121_I2CADDR_DEFAULT, MPR121_NHDT, 0x00);
    twi_writeSingleToPointer(MPR121_TWI_PORT, MPR121_I2CADDR_DEFAULT, MPR121_NCLT, 0x00);
    twi_writeSingleToPointer(MPR121_TWI_PORT, MPR121_I2CADDR_DEFAULT, MPR121_FDLT, 0x00);

    twi_writeSingleToPointer(MPR121_TWI_PORT, MPR121_I2CADDR_DEFAULT, MPR121_DEBOUNCE, 0);
    twi_writeSingleToPointer(MPR121_TWI_PORT, MPR121_I2CADDR_DEFAULT, MPR121_CONFIG1, 0x10);  // default, 16uA charge current
    twi_writeSingleToPointer(MPR121_TWI_PORT, MPR121_I2CADDR_DEFAULT, MPR121_CONFIG2, 0x20);
// #ifdef AUTOCONFIG
    twi_writeSingleToPointer(MPR121_TWI_PORT, MPR121_I2CADDR_DEFAULT, MPR121_AUTOCONFIG0, 0x0B);

    // correct values for Vdd = 3.3V
    twi_writeSingleToPointer(MPR121_TWI_PORT, MPR121_I2CADDR_DEFAULT, MPR121_UPLIMIT, 200);      // ((Vdd - 0.7)/Vdd) * 256
    twi_writeSingleToPointer(MPR121_TWI_PORT, MPR121_I2CADDR_DEFAULT, MPR121_TARGETLIMIT, 180);  // UPLIMIT * 0.9
    twi_writeSingleToPointer(MPR121_TWI_PORT, MPR121_I2CADDR_DEFAULT, MPR121_LOWLIMIT, 130);     // UPLIMIT * 0.65
// #endif

    // enable X electrodes and start MPR121
    uint8_t ECR_SETTING =
        0b10000000 + 12;                                                                         // 5 bits for baseline tracking & proximity disabled + X
                                                                                                 // amount of electrodes running (12)
    twi_writeSingleToPointer(MPR121_TWI_PORT, MPR121_I2CADDR_DEFAULT, MPR121_ECR, ECR_SETTING);  // start with above ECR setting
    return true;
}
void tick_mpr121() {
    if (!mpr121_init && !init_mpr121()) {
        return;
    }
    uint16_t raw;
    if (!twi_readFromPointer(MPR121_TWI_PORT, MPR121_I2CADDR_DEFAULT, MPR121_TOUCHSTATUS_L, sizeof(raw), (uint8_t*)&raw)) {
        mpr121_init = false;
    }
}
#endif