#include <stdio.h>

#include "servo.h"
#include "hardware/adc.h"
#include "pico/stdlib.h"
#include "pico/time.h"

#include "servo_adc.h"
#include "servo_pwm.h"

static button_state_t g_button_state = BUTTON_STATE_IDLE;
static bool g_prev_button = false;
static uint32_t g_press_time = 0;
static bool g_done_event = false;
static bool g_timer_active = false;

void button_state_init(void)
{
    g_button_state = BUTTON_STATE_IDLE;
    g_prev_button = false;
    g_press_time = 0;
    g_done_event = false;
    g_timer_active = false;
}

void button_init(void)
{
    gpio_init(HATCH_GEAR_BUTTON_GPIO);
    gpio_set_dir(HATCH_GEAR_BUTTON_GPIO, GPIO_IN);
    gpio_pull_up(HATCH_GEAR_BUTTON_GPIO); // プルアップ抵抗を有効にする
}

void button_state_update(bool button_now, uint32_t now_ms)
{
    // IDLE中に立ち上がりエッジを検出したら開始
    if (button_now && !g_prev_button && g_button_state == BUTTON_STATE_IDLE) {
        g_press_time = now_ms;
        g_button_state = BUTTON_STATE_WAIT;
        g_timer_active = true;
    }

    // 待機時間経過でDONEイベントを1回だけ出す
    if (g_timer_active) {
        if ((now_ms - g_press_time) >= BUTTON_WAIT_TIME_MS) {
            g_button_state = BUTTON_STATE_DONE;
            g_done_event = true;      // 1回だけ立つイベント
            g_timer_active = false;
        }
    }

    g_prev_button = button_now;
}

button_state_t button_state_get(void)
{
    return g_button_state;
}

static hatch_gear_t g_hatch_gear_timer = HATCH_GEAR_TIMER_IDLE;
static uint32_t hatch_gear_start_time = 0;

void hatch_gear_timer_init(void)
{
    g_hatch_gear_timer = HATCH_GEAR_TIMER_IDLE;
    hatch_gear_start_time = 0;
}

void hatch_gear_pwm_init(void)
{
    servo_pwm_init_us(R_HATCH_PWM, R_HATCH_OPEN);
    servo_pwm_init_us(L_HATCH_PWM, L_HATCH_OPEN);
    servo_pwm_init_us(R_GEAR_PWM, R_GEAR_IDLE);
    servo_pwm_init_us(L_GEAR_PWM, L_GEAR_IDLE);
}

void hatch_gear_timer_update(void)
{
    uint32_t now = now_ms();

    // DONEイベントを1回だけ消費してシーケンス開始
    if (g_done_event) {
        g_done_event = false;

        if (g_hatch_gear_timer == HATCH_GEAR_TIMER_IDLE) {
            g_hatch_gear_timer = HATCH_GEAR_TIMER_SHORTEN;
            hatch_gear_start_time = now;   // ← ここが重要
        }
    }

    switch (g_hatch_gear_timer) {
        case HATCH_GEAR_TIMER_IDLE:
            break;

        case HATCH_GEAR_TIMER_SHORTEN:
            if ((now - hatch_gear_start_time) >= HATCH_GEAR_TIMER_INTERVAL) {
                g_hatch_gear_timer = HATCH_GEAR_TIMER_STORAGE;
                hatch_gear_start_time = now;
            }
            break;

        case HATCH_GEAR_TIMER_STORAGE:
            if ((now - hatch_gear_start_time) >= HATCH_GEAR_TIMER_INTERVAL) {
                g_hatch_gear_timer = HATCH_GEAR_TIMER_L_HATCH;
                hatch_gear_start_time = now;
            }
            break;

        case HATCH_GEAR_TIMER_L_HATCH:
            if ((now - hatch_gear_start_time) >= HATCH_GEAR_TIMER_INTERVAL) {
                g_hatch_gear_timer = HATCH_GEAR_TIMER_R_HATCH;
                hatch_gear_start_time = now;
            }
            break;

        case HATCH_GEAR_TIMER_R_HATCH:
            if ((now - hatch_gear_start_time) >= HATCH_GEAR_TIMER_INTERVAL) {
                g_hatch_gear_timer = HATCH_GEAR_TIMER_FINISH;
                hatch_gear_start_time = now;
            }
            break;

        case HATCH_GEAR_TIMER_FINISH:
            break;

        default:
            break;
    }
}

uint32_t now_ms(void)
{
    return to_ms_since_boot(get_absolute_time());
}

static uint16_t r_hatch_state = R_HATCH_OPEN;
static uint16_t l_hatch_state = L_HATCH_OPEN;
static uint16_t r_gear_state = R_GEAR_IDLE;
static uint16_t l_gear_state = L_GEAR_IDLE;

void hatch_gear_controller(void)
{
    switch (g_hatch_gear_timer) {
        case HATCH_GEAR_TIMER_IDLE:
            break;

        case HATCH_GEAR_TIMER_SHORTEN:
            r_gear_state = R_GEAR_SHORTEN;
            l_gear_state = L_GEAR_SHORTEN;
            break;

        case HATCH_GEAR_TIMER_STORAGE:
            r_gear_state = R_GEAR_STORAGE;
            l_gear_state = L_GEAR_STORAGE;
            break;

        case HATCH_GEAR_TIMER_L_HATCH:
            l_hatch_state = L_HATCH_CLOSE;
            break;

        case HATCH_GEAR_TIMER_R_HATCH:
            r_hatch_state = R_HATCH_CLOSE;
            break;

        case HATCH_GEAR_TIMER_FINISH:
            break;

        default:
            break;
    }

    servo_pwm_write_us(R_HATCH_PWM, r_hatch_state);
    servo_pwm_write_us(L_HATCH_PWM, l_hatch_state);
    servo_pwm_write_us(R_GEAR_PWM, r_gear_state);
    servo_pwm_write_us(L_GEAR_PWM, l_gear_state);
}



void servo_pwm_test(void)
{   
    static int test_pin = 8; // テスト用GPIOピン
    servo_pwm_init_us(test_pin, 1500);
    while (true) {
        servo_pwm_write_us(test_pin, 1000);
        printf("1000us\n");
        sleep_ms(1000);

        servo_pwm_write_us(test_pin, 1500);
        printf("1500us\n");
        sleep_ms(1000);

        servo_pwm_write_us(test_pin, 2000);
        printf("2000us\n");
        sleep_ms(1000);
    }
}

void adc_reader(void)
{
    servo_adc_init(LAD_ADC);
    while (true) {
        uint16_t adc_value = servo_adc_read_avg(LAD_ADC_CHANNEL);
        printf("ADC Value: %u\n", adc_value);
        sleep_ms(500);
    }
}

void tail_setup(void)
{
    servo_adc_init(LAD_ADC);
    servo_adc_init(ELE_ADC);
    // PWMの初期化
    servo_pwm_init_us(LAD_PWM, LAD_NUTRAL); // 50Hzで初期化、初期dutyはニュートラル
    servo_pwm_init_us(ELE_PWM, ELE_NUTRAL);
}

void tail_controller(void)
{
    uint16_t lad_adc_value = servo_adc_read_avg(LAD_ADC_CHANNEL);
    uint16_t lad_pwm_value = adc_to_pwm(lad_adc_value, LAD_ADC_NUTRAL, LAD_ADC_MIN, LAD_ADC_MAX, LAD_DEADZONE, LAD_NUTRAL, LAD_MIN, LAD_MAX, LAD_REVERSAL_FLAG);
    servo_pwm_write_us(LAD_PWM, lad_pwm_value);

    uint16_t ele_adc_value = servo_adc_read_avg(ELE_ADC_CHANNEL);
    uint16_t ele_pwm_value = adc_to_pwm(ele_adc_value, ELE_ADC_NUTRAL, ELE_ADC_MIN, ELE_ADC_MAX, ELE_DEADZONE, ELE_NUTRAL, ELE_MIN, ELE_MAX, ELE_REVERSAL_FLAG);
    servo_pwm_write_us(ELE_PWM, ele_pwm_value);
}


uint16_t adc_to_pwm(
    uint16_t adc_value,
    uint16_t adc_neutral,
    uint16_t adc_min,
    uint16_t adc_max,
    uint16_t deadzone,
    uint16_t pwm_neutral,
    uint16_t pwm_min,
    uint16_t pwm_max,
    bool reversal_flag)
{
    // --- オフセット計算 ---
    int32_t adc_offset = (int32_t)adc_value - (int32_t)adc_neutral;

    // --- 反転 ---
    if (reversal_flag) {
        adc_offset = -adc_offset;
    }

    // --- デッドゾーン ---
    if (adc_offset > -(int32_t)deadzone && adc_offset < (int32_t)deadzone) {
        return pwm_neutral;
    }

    int32_t pwm_value = pwm_neutral;

    // --- 正方向 ---
    if (adc_offset > 0) {
        int16_t adc_range = (int16_t)adc_max - (int16_t)adc_neutral;

        if (adc_range > 0) {
            int16_t pwm_range = (int16_t)pwm_max - (int16_t)pwm_neutral;

            pwm_value = pwm_neutral +
                        (adc_offset * adc_offset * pwm_range) / (adc_range * adc_range); // 二次関数的に変換
        } else {
            pwm_value = pwm_neutral;  // 安全フォールバック
        }
    }
    // --- 負方向 ---
    else {
        int16_t adc_range = (int16_t)adc_min - (int16_t)adc_neutral;

        if (adc_range < 0) {
            int16_t pwm_range = (int16_t)pwm_min - (int16_t)pwm_neutral;

            pwm_value = pwm_neutral +
                        (adc_offset * adc_offset * pwm_range) / (adc_range * adc_range); // 二次関数的に変換
        } else {
            pwm_value = pwm_neutral; 
        }
    }

    // --- クランプ ---
    if (pwm_value > pwm_max) {
        pwm_value = pwm_max;
    } else if (pwm_value < pwm_min) {
        pwm_value = pwm_min;
    }

    return (uint16_t)pwm_value;
}