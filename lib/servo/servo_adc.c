#include "pico/stdlib.h"
#include "hardware/adc.h"
#include <stdint.h>
#include <stdio.h>

#define ADC_HISTORY_SIZE 2
#define ADC_INPUT_COUNT 8

static uint16_t adc_history[ADC_INPUT_COUNT][ADC_HISTORY_SIZE] = {0};
static uint32_t adc_sum[ADC_INPUT_COUNT] = {0};
static uint8_t adc_index[ADC_INPUT_COUNT] = {0};
static uint8_t adc_count[ADC_INPUT_COUNT] = {0};

void servo_adc_init(uint8_t adc_gpio)
{
    adc_init();
    adc_gpio_init(adc_gpio);
}

uint16_t servo_adc_read_raw(uint8_t adc_input)
{
    adc_select_input(adc_input);
    return adc_read();   // 0～4095
}

uint16_t servo_adc_read_avg(uint8_t adc_input)
{
    uint16_t new_value = servo_adc_read_raw(adc_input);

    if (adc_count[adc_input] < ADC_HISTORY_SIZE) {
        adc_history[adc_input][adc_index[adc_input]] = new_value;
        adc_sum[adc_input] += new_value;
        adc_count[adc_input]++;
    } else {
        adc_sum[adc_input] -= adc_history[adc_input][adc_index[adc_input]];
        adc_history[adc_input][adc_index[adc_input]] = new_value;
        adc_sum[adc_input] += new_value;
    }

    adc_index[adc_input] = (adc_index[adc_input] + 1) % ADC_HISTORY_SIZE;

    return (uint16_t)(adc_sum[adc_input] / adc_count[adc_input]);
}