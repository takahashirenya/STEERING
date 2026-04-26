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
#define LAD_ADC 47
#define ELE_ADC 46

#define LAD_ADC_CHANNEL 7
#define ELE_ADC_CHANNEL 6
#define LAD_PWM 22
#define ELE_PWM 28

#define LAD_DEADZONE 100 //ニュートラルの±200の範囲は無視する。
#define ELE_DEADZONE 300 //ニュートラルの±200の範囲は無視する。

#define ELE_ADC_MAX 3780
#define ELE_ADC_NUTRAL 2450
#define ELE_ADC_MIN 1110

#define LAD_ADC_MAX 4060
#define LAD_ADC_NUTRAL 2040
#define LAD_ADC_MIN 10

#define ELE_REVERSAL_FLAG false
#define ELE_MAX 2276
#define ELE_NUTRAL 1793
#define ELE_MIN 1274

#define LAD_REVERSAL_FLAG false
#define LAD_MAX 1860
#define LAD_NUTRAL 1504
#define LAD_MIN 1168


// hatch and gear property

#define R_HATCH_PWM 34
#define L_HATCH_PWM 3

#define R_GEAR_PWM 36
#define L_GEAR_PWM 4

#define R_HATCH_OPEN 1000
#define R_HATCH_CLOSE 2000

#define L_HATCH_OPEN 1000
#define L_HATCH_CLOSE 2000

#define R_GEAR_IDLE 1500
#define R_GEAR_SHORTEN 1000
#define R_GEAR_STORAGE 2000

#define L_GEAR_IDLE 1500
#define L_GEAR_SHORTEN 2000
#define L_GEAR_STORAGE 1000

#define BUTTON_WAIT_TIME_MS 7000U // ボタンが押されてから7秒後にイベント発生
#define HATCH_GEAR_TIMER_INTERVAL 500U  // 500msごとに次の動作に移る

#define HATCH_GEAR_BUTTON_GPIO 39


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

