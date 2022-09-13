#include "eeprom/eeprom.h"
#include "controller/guitar_includes.h"
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
static uint8_t EEMEM test = 0;
static Configuration_t EEMEM config_pointer = DEFAULT_CONFIG;
const Configuration_t PROGMEM default_config = DEFAULT_CONFIG;
void loadConfig(Configuration_t *config) {
  eeprom_read_block(config, &config_pointer, sizeof(Configuration_t));
  // Do this first, as previous controllers will have their config stored in a
  // different location, and then the following changes will be to an invalid
  // config otherwise.
  if (config->main.version < 8) {
    eeprom_read_block(config, &test, sizeof(Configuration_t));
  }
  // Check versions, if they aren't the same, a breaking change has happened
  // Check signatures, that way we know if the EEPROM has a valid config
  // If the signatures don't match, then the EEPROM has garbage data
  if (config->main.signature != ARDWIINO_DEVICE_TYPE) {
    memcpy_P(config, &default_config, sizeof(Configuration_t));
    config->main.version = 0;
  }
  // version 2 adds leds and midi.
  if (config->main.version < 2) {
    memcpy_P(&config->midi, &default_config.midi, sizeof(default_config.midi));
  }
  // Old configs had the subtype for guitars directly, new configs have
  // additional subtypes that get mapped to the guitar subtype
  if (config->main.subType == REAL_GUITAR_SUBTYPE) {
    config->main.subType = XINPUT_GUITAR_HERO_GUITAR;
  }
  // Old configs had the subtype for drums directly, new configs have additional
  // subtypes that get mapped to the drum subtype
  if (config->main.subType == REAL_DRUM_SUBTYPE) {
    config->main.subType = XINPUT_GUITAR_HERO_DRUMS;
  }
  if (config->main.version < 4) {
    memcpy_P(&config->leds, &default_config.leds, sizeof(default_config.leds));
  }
  if (config->main.version < 6) { config->main.pollRate = POLL_RATE; }
  if (config->main.version < 7) { config->rf.rfInEnabled = false; }
  // We made a change to simplify the guitar config, but as a result whammy is
  // now flipped
  if (config->main.version < 9 && isGuitar(config->main.subType)) {
    config->pins.r_x.inverted = !config->pins.r_x.inverted;
  }
  if (config->main.version < 13) {
    memcpy_P(&config->debounce, &default_config.debounce,
             sizeof(default_config.debounce));
    memcpy_P(&config->axisScale, &default_config.axisScale,
             sizeof(default_config.axisScale));
  }
  if (config->main.version < 14) {
    switch (config->axis.mpu6050Orientation) {
    case NEGATIVE_X:
    case POSITIVE_X:
      config->axis.mpu6050Orientation = X;
      break;
    case NEGATIVE_Y:
    case POSITIVE_Y:
      config->axis.mpu6050Orientation = Y;
      break;
    case NEGATIVE_Z:
    case POSITIVE_Z:
      config->axis.mpu6050Orientation = Z;
      break;
    }
  }
  if (config->main.version < 15) { config->debounce.combinedStrum = false; }
  if (config->main.version < 16) {
    config->neck.gh5Neck = false;
    config->neck.gh5NeckBar = false;
    config->neck.wtNeck = false;
    config->neck.wiiNeck = false;
    config->neck.ps2Neck = false;
  }
  if (config->main.version < 17 && config.main.subtype > XINPUT_ARCADE_PAD) {
    config->main.subtype += XINPUT_TURNTABLE - XINPUT_ARCADE_PAD;
  }
  if (config->main.version < CONFIG_VERSION) {
    config->main.version = CONFIG_VERSION;
    eeprom_update_block(config, &config_pointer, sizeof(Configuration_t));
  }
}
void writeConfigByte(uint16_t offset, uint8_t byte) {
  eeprom_update_byte(((uint8_t *)&config_pointer) + offset, byte);
}
void writeConfigBlock(uint16_t offset, const uint8_t *data, uint16_t len) {
  eeprom_update_block(data, ((uint8_t *)&config_pointer) + offset, len);
}
void readConfigBlock(uint16_t offset, uint8_t *data, uint16_t len) {
  eeprom_read_block(data, ((uint8_t *)&config_pointer) + offset, len);
}

void resetConfig(void) {
  Configuration_t config;
  memcpy_P(&config, &default_config, sizeof(Configuration_t));
  eeprom_update_block(&config, &config_pointer, sizeof(Configuration_t));
}