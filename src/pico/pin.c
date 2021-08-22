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
// const Bindings_t* bindingsPointer =
//     (const Bindings_t*)(XIP_BASE + FLASH_TARGET_OFFSET);
uint8_t analogCount = 0;
int currentAnalog = 0;
void initPins(void) {
    setDefaults();
    // Bindings_t test;
    // memset(&test, 0xff, sizeof(test));
    // Bindings_t* bindingsPointer = &test;
    // bindingsPointer->bindings[9].binding = XBOX_DPAD_UP;
    // bindingsPointer->bindings[9].pullup = true;
    // bindingsPointer->bindings[9].pulldown = false;
    // bindingsPointer->bindings[9].milliDeBounce = 20;
    // bindingsPointer->bindings[10].binding = XBOX_DPAD_DOWN;
    // bindingsPointer->bindings[10].pullup = true;
    // bindingsPointer->bindings[10].pulldown = false;
    // bindingsPointer->bindings[10].milliDeBounce = 20;
    // bindingsPointer->bindings[0].binding = XBOX_START;
    // bindingsPointer->bindings[0].pullup = true;
    // bindingsPointer->bindings[0].pulldown = false;
    // bindingsPointer->bindings[0].milliDeBounce = 20;
    // bindingsPointer->bindings[1].binding = XBOX_BACK;
    // bindingsPointer->bindings[1].pullup = true;
    // bindingsPointer->bindings[1].pulldown = false;
    // bindingsPointer->bindings[1].milliDeBounce = 20;
    // bindingsPointer->bindings[11].binding = XBOX_A;
    // bindingsPointer->bindings[11].pullup = true;
    // bindingsPointer->bindings[11].pulldown = false;
    // bindingsPointer->bindings[11].milliDeBounce = 20;
    // bindingsPointer->bindings[12].binding = XBOX_B;
    // bindingsPointer->bindings[12].pullup = true;
    // bindingsPointer->bindings[12].pulldown = false;
    // bindingsPointer->bindings[12].milliDeBounce = 20;
    // bindingsPointer->bindings[13].binding = XBOX_Y;
    // bindingsPointer->bindings[13].pullup = true;
    // bindingsPointer->bindings[13].pulldown = false;
    // bindingsPointer->bindings[13].milliDeBounce = 20;
    // bindingsPointer->bindings[14].binding = XBOX_X;
    // bindingsPointer->bindings[14].pullup = true;
    // bindingsPointer->bindings[14].pulldown = false;
    // bindingsPointer->bindings[14].milliDeBounce = 20;
    // bindingsPointer->bindings[15].binding = XBOX_LB;
    // bindingsPointer->bindings[15].pullup = true;
    // bindingsPointer->bindings[15].pulldown = false;
    // bindingsPointer->bindings[15].milliDeBounce = 20;

    Binding_t* binding = (Binding_t*)buf;
    AnalogData_t* analog = (AnalogData_t*)buf + sizeof(Binding_t);
    for (int i = 0; i < PORTS * PINS_PER_PORT; i++) {
        binding = &bindingsPointer->bindings[i];
        if (binding->binding != 0xFF) {
            pins[i].offset = i;
            pins[i].binding = binding->binding;
            pins[i].pullup = binding->pullup;
            pins[i].milliDeBounce = binding->milliDeBounce;
            pins[i].digitalRead = readDigital;
            if (binding->analogID != 0xFF) {
                analog = &bindingsPointer->analog[binding->analogID];
                pins[i].axisInfo = &analogInfo[binding->analogID];
                pins[i].axisInfo->deadzone = analog->scale.deadzone;
                pins[i].axisInfo->offset = analog->scale.offset;
                pins[i].axisInfo->multiplier = analog->scale.multiplier;
                pins[i].axisInfo->isADC = analog->mapToDigital;
                pins[i].axisInfo->ADCtrigger = analog->trigger;
                pins[i].axisInfo->channel = analog->channel;
                pins[i].axisInfo->analogRead = readAnalog;
                adc_gpio_init(i);
            } else {
                pins[i].axisInfo = NULL;
                gpio_init(i);
                gpio_set_dir(i, false);
                gpio_set_pulls(i, binding->pullup, binding->pulldown);
            }
        }
    }
    adc_init();
}
void setDefaults(void) {
    // 0xFF is used everywhere as a flag for something being disabled.
    Bindings_t clear;
    memset(&clear, 0xff, sizeof(clear));
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