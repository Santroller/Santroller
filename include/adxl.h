#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define ADXL345_ADDRESS 0x53
#define ADXL345_POWER_CTL 0x2D
#define ADXL345_DATA_FORMAT 0x31
#define ADXL345_DATAX0 0x32
#define ADXL345_GRAVITY_EARTH        9.80665f
#define LOW_PASS_ALPHA 0.5
void tick_adxl();
void init_adxl();
#ifdef __cplusplus
}
#endif