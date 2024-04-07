#include "config.h"
#include "io.h"
#include "shared_main.h"
#include "max170x.h"
#include "Arduino.h"
#ifdef MAX1704X_TWI_PORT
bool max170x_init = false;
uint8_t lastBattery = 0;
long init_time_millis = 0;
long last_update_millis = 0;
bool write16(uint8_t addr, uint16_t data) {
    return twi_writeToPointer(MAX1704X_TWI_PORT, MAX710X_I2C_ADDRESS, addr, sizeof(data), (uint8_t*)&data);
}
bool write8(uint8_t addr, uint8_t data) {
    return twi_writeSingleToPointer(MAX1704X_TWI_PORT, MAX710X_I2C_ADDRESS, addr, data);
}
bool init_max170x() {
    write16(REGISTER_COMMAND, RESET_COMMAND);
    uint16_t data;
    twi_readFromPointer(MAX1704X_TWI_PORT, MAX710X_I2C_ADDRESS, REGISTER_CONFIG, sizeof(data), (uint8_t*)&data);
    if (data != 0x1C97) {
        return false;
    }
    sleep_ms(10);
    write16(REGISTER_MODE, QUICKSTART_MODE);
    init_time_millis = millis();
    max170x_init = true;

    return true;
}
void tick_max170x() {
    if (!max170x_init && !init_max170x()) {
        return;
    }
    long m = millis();
    // give 1 second to initialize (datasheet only says 500ms), update, then only update once a minute 
    if ((m - init_time_millis) < 1000 || (last_update_millis != 0 && (m - last_update_millis < 60000))) {
        return;
    }
    uint8_t raw;
    if (!twi_readFromPointer(MAX1704X_TWI_PORT, MAX710X_I2C_ADDRESS, REGISTER_SOC, sizeof(raw), (uint8_t*)&raw)) {
        max170x_init = false;
    }
#ifdef BLUETOOTH_TX
    if (raw > 100) {
        raw = 100;
    }
    set_battery_state(raw);
    lastBattery = raw;
    last_update_millis = m;
#endif
}
#endif