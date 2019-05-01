#include "eeprom.h"
config_t EEMEM config_pointer = {
    PROTOCOL_VERSION,     OUTPUT_TYPE,    DEVICE_TYPE, TILT_SENSOR,
    XINPUT_SUBTYPE,       POLL_RATE,      PINS,        DIRECTION_MODE,
    WHAMMY_INITIAL_VALUE, MPU_6050_START, FRETS_LED,   KEYS};

config_t config;