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
  // Check signatures, that way we know if the EEPROM has a valid config
  // If the signatures don't match, then the EEPROM has garbage data, so we just load defaults and return
  if (config->main.signature != ARDWIINO_DEVICE_TYPE) {
    memcpy_P(config, &default_config, sizeof(Configuration_t));
    writeConfigBlock(0, (uint8_t *)config, sizeof(Configuration_t));
    return;
  }
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
  if (config->main.version < 17 && config->main.subType > XINPUT_ARCADE_PAD) {
    config->main.subType += XINPUT_TURNTABLE - XINPUT_ARCADE_PAD;
    if (config->main.subType > PS3_GAMEPAD) {
      config->main.subType += 1; 
    }
  }

  if (config->main.version < 18) {
    config->deque = false;
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