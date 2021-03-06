#include "eeprom/eeprom.h"
#include "hardware/flash.h"
#include "pico/stdlib.h"
#include "util/util.h"
#include <string.h>
Configuration_t config;
const Configuration_t default_config = DEFAULT_CONFIG;
const __attribute__((section(".ardCfg"))) Configuration_t flash_target_contents;
Configuration_t newConfig;
void loadConfig(void) {
  config = flash_target_contents;
  if (config.main.signature != ARDWIINO_DEVICE_TYPE) {
    config = default_config;
    config.main.version = 0;
  }
  if (config.main.version < CONFIG_VERSION) {
    config.main.version = CONFIG_VERSION;
    writeConfigBlock(0, (uint8_t *)&config, sizeof(Configuration_t));
  }
  newConfig = config;
}
void writeConfigBlock(uint8_t offset, const uint8_t *data, uint8_t len) {
  uint8_t *c = (uint8_t *)&newConfig;
  memcpy(c + offset, data, len);
  if (offset + len >= sizeof(Configuration_t)) {
    cli();
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, c, FLASH_PAGE_SIZE);
    sei();
  }
}

void resetConfig(void) {
  writeConfigBlock(0, (uint8_t *)&default_config, sizeof(Configuration_t));
}