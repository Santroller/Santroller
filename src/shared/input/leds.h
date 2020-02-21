#ifdef __cplusplus
extern "C" {
#endif
#include "input_handler.h"
#include "../config/eeprom.h"
#define NUM_LEDS 5
void led_init(void);
void led_tick(controller_t *controller);
#ifdef __cplusplus
}
#endif