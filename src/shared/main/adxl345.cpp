#include "io.h"
#include "adxl.h"
#include "fxpt_math.h"
#include "config.h"
static int16_t filtered[3] = {0};
uint16_t pitch;
uint16_t roll;
#ifdef INPUT_ADXL
void init_adxl() {
    twi_writeSingleToPointer(ADXL_TWI_PORT, ADXL345_ADDRESS, ADXL345_POWER_CTL, 0x08);
    twi_writeSingleToPointer(ADXL_TWI_PORT, ADXL345_ADDRESS, ADXL345_DATA_FORMAT, 0x0B);
}
void tick_adxl() {
    int16_t raw[3];
    twi_readFromPointer(ADXL_TWI_PORT, ADXL345_ADDRESS, ADXL345_DATAX0, 6, (uint8_t*)raw);
    for (int i = 0; i < 3; i++) {
        raw[i] = raw[i] * 0.004 * ADXL345_GRAVITY_EARTH;
        filtered[i] = raw[i] * LOW_PASS_ALPHA + (filtered[i] * (1.0 - LOW_PASS_ALPHA));
    }
    roll  = fxpt_atan2(filtered[1], filtered[2]);
    pitch = (fxpt_atan2(-filtered[0], sqrt(filtered[1]*filtered[1] + filtered[2] * filtered[2])));
}
#endif