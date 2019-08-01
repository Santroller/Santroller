#include "eeprom.h"
config_t EEMEM config_pointer = {PROTOCOL_VERSION,
                                 DEVICE_TYPE,
                                 OUTPUT_TYPE,
                                 TILT_SENSOR,
                                 POLL_RATE,
                                 PINS,
                                 MPU_6050_ORIENTATION,
                                 FRETS_LED,
                                 MAP_JOY_TO_DPAD,
                                 MAP_START_SELECT_TO_HOME,
                                 INVERSIONS,
                                 TRIGGER_THRESHOLD,
                                 JOY_THRESHOLD,
                                 KEYS,
                                 F_CPU,
                                 FIRMWARE};
config_t config;
void load_config(void) {
    eeprom_read_block(&config, &config_pointer, sizeof(config_t));
}