#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "servo.h"
#include "servo_adc.h"
#include "servo_pwm.h"

int main(void)
{
    stdio_init_all();
    sleep_ms(2000); // USBシリアル安定待ち

    printf("System start\n");

    // ボタン状態管理初期化
    button_state_init();
    button_init();

    // ハッチ・ギアのタイマー初期化
    hatch_gear_timer_init();

    // 尾翼サーボ初期化
    tail_setup();

    hatch_gear_pwm_init();

    while (true) {
        uint32_t now = now_ms();

        // プルアップ入力なので、押したときLOWなら反転する
        bool button_now = (gpio_get(HATCH_GEAR_BUTTON_GPIO) == 0);

        // ボタン状態更新
        button_state_update(button_now, now);

        // ハッチ・ギアの時系列制御更新
        hatch_gear_timer_update();

        // ハッチ・ギア出力反映
        hatch_gear_controller();

        // 尾翼制御
        tail_controller();

        sleep_ms(10);
    }

    return 0;
}