#include "eeprom.h"
config_t EEMEM config_pointer = {PROTOCOL_VERSION,
                                 OUTPUT_TYPE,
                                 DEVICE_TYPE,
                                 TILT_SENSOR,
                                 SUB_TYPE,
                                 POLL_RATE,
                                 PINS,
                                 DIRECTION_MODE,
                                 WHAMMY_INITIAL_VALUE,
                                 MPU_6050_START,
                                 FRETS_LED,
                                 MAP_JOY_TO_DPAD,
                                 TRIGGER_THRESHOLD,
                                 JOY_THRESHOLD,
                                 KEYS,
                                 F_CPU,
                                 0xFEA123};

config_t config;