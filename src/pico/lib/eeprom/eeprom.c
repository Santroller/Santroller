#include "eeprom/eeprom.h"
#include "hardware/flash.h"
#include "pico/stdlib.h"
#include "util/util.h"
#include <string.h>
Configuration_t config;
const Configuration_t default_config = DEFAULT_CONFIG;
const uint8_t *flash_target_contents =
    (const uint8_t *)(XIP_BASE + FLASH_TARGET_OFFSET);
uint8_t newConfig[FLASH_PAGE_SIZE];
void loadConfig(void) {
  memcpy(&config, flash_target_contents, sizeof(Configuration_t));
  if (config.main.signature != ARDWIINO_DEVICE_TYPE) {
    config = default_config;
    config.main.version = 0;
  }
  // We made a change to simplify the guitar config, but as a result whammy is
  // now flipped
  if (config.main.version < 9 && isGuitar(config.main.subType)) {
    config.pins.r_x.inverted = !config.pins.r_x.inverted;
  }
  if (config.main.version < 12) {
    memcpy_P(&config.axisScale, &default_config.axisScale,
             sizeof(default_config.axisScale));
  }
  if (config.main.version < 13) {
    memcpy_P(&config.debounce, &default_config.debounce, sizeof(default_config.debounce));
  }
  if (config.main.version < CONFIG_VERSION) {
    config.main.version = CONFIG_VERSION;
    writeConfigBlock(0, (uint8_t *)&config, sizeof(Configuration_t));
  }
  memcpy(newConfig, &config, sizeof(Configuration_t));
}
void writeConfigBlock(uint8_t offset, const uint8_t *data, uint8_t len) {
  memcpy(newConfig + offset, data, len);
  uint32_t saved_irq;
  if (offset + len >= sizeof(Configuration_t)) {
    saved_irq = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, newConfig, FLASH_PAGE_SIZE);
    restore_interrupts(saved_irq);
  }
}

void resetConfig(void) {
  writeConfigBlock(0, (uint8_t *)&default_config, sizeof(Configuration_t));
}