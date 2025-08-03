#include <stdint.h>
void tud_set_rumble_cb(uint8_t left, uint8_t right);
void tud_set_player_led_cb(uint8_t player);
void tud_set_euphoria_led_cb(uint8_t led);
void tud_set_stage_kit_cb(uint8_t command, uint8_t param);