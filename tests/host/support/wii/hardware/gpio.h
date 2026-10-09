#pragma once
// Fake hardware/gpio.h: remembers the last level put on each pin
#include <stdint.h>
#include <stdbool.h>
enum gpio_function
{
    GPIO_FUNC_I2C = 3,
};
#define GPIO_OUT 1
#define GPIO_IN 0
void gpio_init(unsigned int gpio);
void gpio_set_function(unsigned int gpio, enum gpio_function fn);
void gpio_pull_up(unsigned int gpio);
void gpio_set_dir(unsigned int gpio, bool out);
void gpio_put(unsigned int gpio, bool value);
namespace fake_gpio
{
extern bool level[64];
}
