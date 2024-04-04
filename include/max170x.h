#define MAX710X_I2C_ADDRESS 0x36

#define REGISTER_VCELL      0x02
#define REGISTER_SOC        0x04
#define REGISTER_MODE       0x06
#define REGISTER_VERSION    0x08
#define REGISTER_CONFIG     0x0C
#define REGISTER_COMMAND    0xFE

#define RESET_COMMAND       0x5400
#define QUICKSTART_MODE     0x4000


#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void tick_max170x();
bool init_max170x();
#ifdef __cplusplus
}
#endif