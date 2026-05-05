#pragma once

#include <stdint.h>
#include <stdbool.h>

//set dig property

#define SET_DIG_PWM 32
#define SET_DIG_NUTRAL 1500
#define SLOW_STEP 1
#define FAST_STEP 10

// servoのusの範囲は
//HPS703は、1000-2500くらい
//DS系は、500-2500くらい

// servo property


// tail property
#define LAD_ADC 29
#define ELE_ADC 28

#define LAD_ADC_CHANNEL 3
#define ELE_ADC_CHANNEL 2

#define LAD_PWM 3
#define ELE_PWM 4

#define LAD_DEADZONE 100 //ニュートラルの±200の範囲は無視する。
#define ELE_DEADZONE 300 //ニュートラルの±200の範囲は無視する。

#define ELE_ADC_MAX 3600
#define ELE_ADC_NUTRAL 2450
#define ELE_ADC_MIN 1110

#define LAD_ADC_MAX 3300
#define LAD_ADC_NUTRAL 2040
#define LAD_ADC_MIN 800

#define ELE_REVERSAL_FLAG false
#define ELE_MAX 2129
#define ELE_NUTRAL 1655
#define ELE_MIN 1000

#define LAD_REVERSAL_FLAG true
#define LAD_MAX 1834
#define LAD_NUTRAL 1481
#define LAD_MIN 1148


// hatch and gear property

#define R_HATCH_PWM 23
#define L_HATCH_PWM 1

#define R_GEAR_PWM 24
#define L_GEAR_PWM 2

#define R_HATCH_OPEN 1500
#define R_HATCH_CLOSE 2060

#define L_HATCH_OPEN 2370
#define L_HATCH_CLOSE 1860

#define R_GEAR_IDLE 2340
#define R_GEAR_SHORTEN 1820
#define R_GEAR_STORAGE 1090

#define L_GEAR_IDLE 1240
#define L_GEAR_SHORTEN 1840
#define L_GEAR_STORAGE 2400

#define BUTTON_WAIT_TIME_MS 7000U // ボタンが押されてから7秒後にイベント発生
#define HATCH_GEAR_TIMER_INTERVAL 700U  // 700msごとに次の動作に移る

#define HATCH_GEAR_BUTTON_GPIO 25


typedef enum {
    BUTTON_STATE_IDLE = 0,     // 待機中
    BUTTON_STATE_PRESSED,      // 押された直後
    BUTTON_STATE_WAIT,      // 7秒待機中
    BUTTON_STATE_DONE       // 7秒経過
} button_state_t;

typedef enum {
    HATCH_GEAR_TIMER_IDLE = 0,
    HATCH_GEAR_TIMER_SHORTEN,
    HATCH_GEAR_TIMER_STORAGE,
    HATCH_GEAR_TIMER_L_HATCH,
    HATCH_GEAR_TIMER_R_HATCH,
    HATCH_GEAR_TIMER_FINISH
} hatch_gear_t;

#ifdef __cplusplus
extern "C" // for C++ compilers
{
#endif
    // test functions
    void servo_pwm_test(void);
    void adc_reader(void);
    void set_servo_dig_controller(void);

    // main setup functions
    void tail_setup(void);
    void hatch_gear_pwm_init(void);
    void hatch_gear_timer_init(void);
    void button_state_init(void);
    void button_init(void);
    void set_servo_dig_init(void);

    // main control functions 
    void tail_controller(void);
    void hatch_gear_timer_update(void);
    void hatch_gear_controller(void);


    // utility functions
    uint16_t adc_to_pwm(uint16_t adc_value, uint16_t adc_neutral, uint16_t adc_min, uint16_t adc_max,uint16_t deadzone, uint16_t pwm_neutral, uint16_t pwm_min, uint16_t pwm_max, bool reversal_flag);

    void button_state_update(bool button_now, uint32_t now_ms);
    button_state_t button_state_get(void);
    bool button_state_take_done(void);
    uint32_t now_ms(void);

#ifdef __cplusplus
}
#endif

