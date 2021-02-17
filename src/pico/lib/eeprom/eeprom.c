#include "eeprom/eeprom.h"
#include "hardware/flash.h"
#include "pico/stdlib.h"
#include <string.h>
Configuration_t config;
const Configuration_t default_config = DEFAULT_CONFIG;
// We're going to store the config at a region 256k from the start of flash.
// Once done, we can access this at XIP_BASE + 256k.
#define FLASH_TARGET_OFFSET (256 * 1024)
const Configuration_t *flash_target_contents =
    (const Configuration_t *)(XIP_BASE + FLASH_TARGET_OFFSET);
void loadConfig(void) {
  config = *flash_target_contents;
  if (config.main.signature != ARDWIINO_DEVICE_TYPE) {
    config = default_config;
    config.main.version = 0;
  }
  if (config.main.version < CONFIG_VERSION) {
    config.main.version = CONFIG_VERSION;
    writeConfigBlock(0, (uint8_t *)&config, sizeof(Configuration_t));
  }
}
void writeConfigBlock(uint8_t offset, const uint8_t *data, uint8_t len) {
  uint8_t output[256] = {0};
  memcpy(output, flash_target_contents, sizeof(Configuration_t));
  memcpy(output + offset, data, len);
  flash_range_erase(FLASH_TARGET_OFFSET, FLASH_PAGE_SIZE);
  flash_range_program(FLASH_TARGET_OFFSET, output, FLASH_PAGE_SIZE);
}

void resetConfig(void) {
  writeConfigBlock(0, (uint8_t *)&default_config, sizeof(Configuration_t));
}