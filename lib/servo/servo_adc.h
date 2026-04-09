#ifndef SERVO_ADC_H
#define SERVO_ADC_H

#include <stdint.h>

void servo_adc_init(uint8_t adc_gpio);
uint16_t servo_adc_read_raw(uint8_t adc_input);
uint16_t servo_adc_read_avg(uint8_t adc_input);

#endif