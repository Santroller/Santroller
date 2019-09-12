#include "eeprom.h"
config_t EEMEM config_pointer = DEFAULT_CONFIG;
config_t config;
void load_config(void) {
    eeprom_read_block(&config, &config_pointer, sizeof(config_t));
}
void write_config(void) {
    eeprom_write_block(&config, &config_pointer, sizeof(config_t));
}