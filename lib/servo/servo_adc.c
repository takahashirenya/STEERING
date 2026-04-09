#include "pico/stdlib.h"
#include "hardware/adc.h"
#include <stdint.h>
#include <stdio.h>

#define ADC_HISTORY_SIZE 2

static uint16_t adc_history[ADC_HISTORY_SIZE] = {0};
static uint32_t adc_sum = 0;
static uint8_t adc_index = 0;
static uint8_t adc_count = 0;

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

    // まだバッファが埋まっていない間
    if (adc_count < ADC_HISTORY_SIZE) {
        adc_history[adc_index] = new_value;
        adc_sum += new_value;
        adc_count++;
    } 
    // 埋まった後は古い値を引いて新しい値を足す
    else {
        adc_sum -= adc_history[adc_index];
        adc_history[adc_index] = new_value;
        adc_sum += new_value;
    }

    adc_index = (adc_index + 1) % ADC_HISTORY_SIZE;

    return (uint16_t)(adc_sum / adc_count);
}