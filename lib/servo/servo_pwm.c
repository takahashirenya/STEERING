#include "servo_pwm.h"
#include "pico/stdlib.h"
#include "hardware/pwm.h"

#define PWM_CLOCK_HZ 125000000.0f
#define SERVO_WRAP 19999     // 20ms周期
#define SERVO_CLKDIV 125.0f  // 125MHz / 125 = 1MHz -> 1count = 1us

bool servo_pwm_init_us(uint16_t gpio, uint16_t default_us)
{
    gpio_set_function(gpio, GPIO_FUNC_PWM);

    uint slice = pwm_gpio_to_slice_num(gpio);

    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, SERVO_CLKDIV);
    pwm_config_set_wrap(&config, SERVO_WRAP);

    pwm_init(slice, &config, true);

    uint channel = pwm_gpio_to_channel(gpio);
    pwm_set_chan_level(slice, channel, default_us);  // usをそのまま使える

    return true;
}

void servo_pwm_write_us(uint16_t gpio, uint16_t us)
{
    if (us < 500) us = 500;
    if (us > 2500) us = 2500;

    uint slice = pwm_gpio_to_slice_num(gpio);
    uint channel = pwm_gpio_to_channel(gpio);
    pwm_set_chan_level(slice, channel, us);
}