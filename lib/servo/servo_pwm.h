#ifndef SERVO_PWM_H
#define SERVO_PWM_H

#include <stdint.h>
#include <stdbool.h>

bool servo_pwm_init_us(uint16_t gpio, uint16_t default_us);
void servo_pwm_write_us(uint16_t gpio, uint16_t us);

#endif