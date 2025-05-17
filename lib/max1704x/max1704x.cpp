#include "max1704x.hpp"

bool Max1704X::init() {
    write16(REGISTER_COMMAND, RESET_COMMAND);
    uint16_t data;
    interface->writeRegister(MAX710X_I2C_ADDRESS, REGISTER_CONFIG, sizeof(data), (uint8_t*)&data);
    if (data != 0x1C97) {
        return false;
    }
    sleep_ms(10);
    write16(REGISTER_MODE, QUICKSTART_MODE);
    connected = true;

    return true;
}
void Max1704X::tick() {
    if (!connected && !init()) {
        return;
    }
    uint8_t raw;
    if (!interface->readRegister(MAX710X_I2C_ADDRESS, REGISTER_SOC, sizeof(raw), (uint8_t*)&raw)) {
        connected = false;
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