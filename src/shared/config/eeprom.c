#include "eeprom.h"
config_t EEMEM config_pointer = DEFAULT_CONFIG;
config_t default_config = DEFAULT_CONFIG;
config_t config;
void load_config(void) {
    eeprom_read_block(&config, &config_pointer, sizeof(config_t));
    // Check versions, if they aren't the same, a breaking change has happened
    // Check signatures, that way we know if the EEPROM has a valid config
    // If the signatures don't match, then the EEPROM has garbage data
    if (config.main.signature != ARDWIINO_DEVICE_TYPE) {
        eeprom_write_block(&default_config, &config_pointer, sizeof(config_t));
        config = default_config;
    }
    if (config.main.version == 0) {
        config.new_items.threshold_drums = DRUM_THRESHOLD;
        config.main.version = 1;
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