#include "io.h"
#include "adxl.h"
#include "fxpt_math.h"
#include "config.h"
int16_t filtered[3] = {0};
double currentLowPassAlpha = LOW_PASS_ALPHA;
#ifdef INPUT_ADXL
void init_adxl() {
    twi_writeSingleToPointer(ADXL_TWI_PORT, ADXL345_ADDRESS, ADXL345_POWER_CTL, 0x08);
    twi_writeSingleToPointer(ADXL_TWI_PORT, ADXL345_ADDRESS, ADXL345_DATA_FORMAT, 0x0B);
}
void tick_adxl() {
    int16_t raw[3];
    twi_readFromPointer(ADXL_TWI_PORT, ADXL345_ADDRESS, ADXL345_DATAX0, 6, (uint8_t*)raw);
    for (int i = 0; i < 3; i++) {
        filtered[i] = (raw[i] * 64) * currentLowPassAlpha + (filtered[i] * (1.0 - currentLowPassAlpha));
    }
}
#endif