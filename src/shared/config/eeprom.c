#include "eeprom.h"
#include <avr/pgmspace.h>
config_t EEMEM config_pointer = DEFAULT_CONFIG;
const config_t PROGMEM default_config = DEFAULT_CONFIG;
config_t config;
void load_config(void) {
    eeprom_read_block(&config, &config_pointer, sizeof(config_t));
    // Check versions, if they aren't the same, a breaking change has happened
    // Check signatures, that way we know if the EEPROM has a valid config
    // If the signatures don't match, then the EEPROM has garbage data
    if (config.main.signature != ARDWIINO_DEVICE_TYPE) {
        memcpy_P(&config, &default_config, sizeof(config_t));
        eeprom_write_block(&config, &config_pointer, sizeof(config_t));
    }
    // In version 1 drum thresholds were added, but incorect, version two fixes them and adds leds.
    if (config.main.version < 2) {
        config.new_items.threshold_drums = DRUM_THRESHOLD;
        memcpy_P(&config.new_items.leds, &default_config.new_items.leds, sizeof(default_config.new_items.leds));
        config.main.version = 2;
        write_config();
    }
    if (config.main.sub_type == REAL_GUITAR_SUBTYPE) {
        config.main.sub_type = XINPUT_GUITAR_HERO_GUITAR;
        write_config();
    }
    if (config.main.sub_type == REAL_DRUM_SUBTYPE) {
        config.main.sub_type = XINPUT_GUITAR_HERO_DRUMS;
        write_config();
    }
}
void write_config(void) {
    eeprom_write_block(&config, &config_pointer, sizeof(config_t));
}