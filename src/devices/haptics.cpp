#include "hardware/adc.h"
#include "hardware/pwm.h"
void init() {

    gpio_init(7);
    gpio_set_dir(7, true);
    gpio_put(7, false);
    gpio_init(6);
    gpio_set_dir(6, true);
    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_wrap(&cfg, 1000);
    pwm_init(pwm_gpio_to_slice_num(6), &cfg, true);
    gpio_set_function(6, GPIO_FUNC_PWM);
    pwm_set_gpio_level(6, 600);
}

void tick(bool strum) {
    gpio_put(7, strum);
}