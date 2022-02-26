#include "pin.h"

#include <stdio.h>

#include "config.h"
#include "hardware/adc.h"
#include "hardware/flash.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "lib_main.h"
#include "pico/stdlib.h"
#include "stddef.h"
#include "string.h"
const Bindings_t* bindingsPointer =
    (const Bindings_t*)(XIP_BASE + FLASH_TARGET_OFFSET);
uint8_t analogCount = 0;
int currentAnalog = 0;
void initPins(void) {
    // By resetting analogCount we can run this function again when the config is modified
    pinCount = 0;
    Binding_t* binding;
    AnalogData_t* analog;
    bool isMouse = true;
    // For KBD/MOUSE, we want to make sure that mouse bindings appear first.
    // To do this, we loop twice, once skipping mouse bindings and once skipping keyboard bindings.
    while (true) {
        for (int i = 0; i < PORTS * PINS_PER_PORT; i++) {
            binding = &bindingsPointer->bindings[i];
            if (isMouse != (binding->type == DIRECT_MOUSE)) continue;
            if (binding->type) {
                pins[pinCount].offset = i;
                pins[pinCount].binding = binding->binding;
                pins[pinCount].pullup = binding->pullup;
                pins[pinCount].milliDeBounce = binding->milliDeBounce;
                pins[i].isExt = binding->type == DIRECT_TYPE || binding->type == DIRECT_MOUSE;
                if (pins[i].isExt) {
                    pins[pinCount].digitalRead = readDigital;
                    if (binding->analogID) {
                        analog = &bindingsPointer->analog[binding->analogID];
                        pins[pinCount].axisInfo = &analogInfo[binding->analogID];
                        pins[pinCount].axisInfo->deadzone = analog->scale.deadzone;
                        pins[pinCount].axisInfo->offset = analog->scale.offset;
                        pins[pinCount].axisInfo->multiplier = analog->scale.multiplier;
                        pins[pinCount].axisInfo->isADC = analog->mapToDigital;
                        pins[pinCount].axisInfo->ADCtrigger = analog->trigger;
                        pins[pinCount].axisInfo->channel = analog->channel;
                        pins[pinCount].axisInfo->analogRead = readAnalog;
                        adc_gpio_init(i);
                    } else {
                        gpio_init(i);
                        gpio_set_dir(i, false);
                        gpio_set_pulls(i, binding->pullup, binding->pulldown);
                    }
                }
                pinCount++;
            }
        }
        if (!isMouse || consoleType != KEYBOARD_MOUSE) {
            break;
        }
        firstKeyboardPin = pinCount;
        isMouse = false;
    }
    adc_init();
}
void setDefaults(void) {
    Bindings_t clear;
    memset(&clear, 0x00, sizeof(clear));
    uint32_t saved_irq;
    saved_irq = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, (uint8_t*)&clear, sizeof(clear));
    restore_interrupts(saved_irq);
}

bool readDigital(Input_t* pin) {
    if (pin->axisInfo) {
        return readAnalog(pin) > pin->axisInfo->ADCtrigger;
    }
    return (gpio_get(pin->offset) != 0) != pin->pullup;
}

int16_t readAnalog(Input_t* pin) {
    if (!pin->axisInfo) {
        return readDigital(pin) * INT16_MAX;
    }
    adc_select_input(pin->offset);
    // We have everything coded assuming 10 bits (as that is what the arduino
    // uses) so shift accordingly (12 -> 10)
    return adc_read() >> 2;
}