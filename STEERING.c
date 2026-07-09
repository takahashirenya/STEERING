#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "mcp2515.h"

#include "servo.h"
#include "servo_adc.h"
#include "servo_pwm.h"


#define CAN_SPI         spi1
#define CAN_SCK_PIN     10
#define CAN_MOSI_PIN    11
#define CAN_MISO_PIN    12
#define CAN_CS_PIN      9
#define CAN_INT_PIN     13
#define CAN_BITRATE     MCP2515_BITRATE_1000KBPS
#define CAN_CLOCK       MCP2515_CLOCK_8MHZ
#define CAN_SPI_BAUD    (10 * 1000 * 1000)

#define CAN_ID_ELE      0x001
#define CAN_ID_LAD      0x002
#define CAN_ID_BUTTON   0x003
#define CAN_TIMEOUT_MS  100U


// CAN受信データ保存用
static uint8_t can_ele_data[8];
static uint8_t can_lad_data[8];
static uint8_t can_button_data[8];

static uint8_t can_ele_dlc = 0;
static uint8_t can_lad_dlc = 0;
static uint8_t can_button_dlc = 0;

static bool can_ele_received = false;
static bool can_lad_received = false;
static bool can_button_received = false;

static uint32_t can_ele_last_ms = 0;
static uint32_t can_lad_last_ms = 0;
static uint32_t can_button_last_ms = 0;


static bool is_target_id(uint32_t id)
{
    return id == CAN_ID_ELE || id == CAN_ID_LAD || id == CAN_ID_BUTTON;
}


static void copy_can_data(uint8_t *dst, uint8_t *dst_dlc, const can_frame_t *frame)
{
    *dst_dlc = frame->dlc;

    for (uint8_t i = 0; i < frame->dlc && i < 8; i++) {
        dst[i] = frame->data[i];
    }
}


static void can_read_process(mcp2515_t *can, uint32_t now)
{
    while (mcp2515_check_receive(can)) {
        can_frame_t frame;
        mcp2515_error_t err = mcp2515_read_message(can, &frame);

        if (err != MCP2515_OK) {
            continue;
        }

        // 標準IDだけ使う
        if (frame.extended) {
            continue;
        }

        // 必要なIDだけ読む
        if (!is_target_id(frame.id)) {
            continue;
        }

        if (frame.id == CAN_ID_ELE) {
            copy_can_data(can_ele_data, &can_ele_dlc, &frame);
            can_ele_received = true;
            can_ele_last_ms = now;
        }

        else if (frame.id == CAN_ID_LAD) {
            copy_can_data(can_lad_data, &can_lad_dlc, &frame);
            can_lad_received = true;
            can_lad_last_ms = now;
        }

        else if (frame.id == CAN_ID_BUTTON) {
            copy_can_data(can_button_data, &can_button_dlc, &frame);
            can_button_received = true;
            can_button_last_ms = now;
        }
    }
}

static bool can_data_is_fresh(bool received, uint32_t last_ms, uint32_t now)
{
    return received && (now - last_ms) <= CAN_TIMEOUT_MS;
}

static uint16_t can_data_to_u16(const uint8_t *data)
{
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

int main(void)
{
    stdio_init_all();
    sleep_ms(2000);

    mcp2515_t can;

    mcp2515_init_struct(&can,
                        CAN_SPI,
                        CAN_SCK_PIN,
                        CAN_MOSI_PIN,
                        CAN_MISO_PIN,
                        CAN_CS_PIN,
                        CAN_INT_PIN,
                        CAN_SPI_BAUD);

    mcp2515_error_t ret = mcp2515_begin(&can, CAN_BITRATE, CAN_CLOCK);

    if (ret != MCP2515_OK) {
        while (1) {
            sleep_ms(1000);
        }
    }

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

        // CAN読み取り
        can_read_process(&can, now);

        // ここで受信したCANデータを使う
        //
        // CAN_ID_ELE のデータ:
        // can_ele_data[0] ～ can_ele_data[7]
        //
        // CAN_ID_LAD のデータ:
        // can_lad_data[0] ～ can_lad_data[7]
        //
        // CAN_ID_BUTTON のデータ:
        // can_button_data[0] ～ can_button_data[7]

        // ローカルボタンを使う場合
        bool button_now = (gpio_get(HATCH_GEAR_BUTTON_GPIO) == 0);

        // もしCANボタンを使うなら、例えばこう
        if (can_data_is_fresh(can_button_received, can_button_last_ms, now) && can_button_dlc >= 1) {
            button_now = can_button_data[0] != 0;
        }

        // ボタン状態更新
        button_state_update(button_now, now);

        // ハッチ・ギアの時系列制御更新
        hatch_gear_timer_update();

        // ハッチ・ギア出力反映
        hatch_gear_controller();

        // 尾翼制御
        bool use_lad_adc = can_data_is_fresh(can_lad_received, can_lad_last_ms, now) && can_lad_dlc >= 2;
        bool use_ele_adc = can_data_is_fresh(can_ele_received, can_ele_last_ms, now) && can_ele_dlc >= 2;
        uint16_t lad_adc_value = use_lad_adc ? can_data_to_u16(can_lad_data) : 0;
        uint16_t ele_adc_value = use_ele_adc ? can_data_to_u16(can_ele_data) : 0;
        tail_controller_with_adc_values(lad_adc_value, use_lad_adc, ele_adc_value, use_ele_adc);
        printf("lad_adc: %u, use_lad: %d, ele_adc: %u, use_ele: %d\n", lad_adc_value, use_lad_adc, ele_adc_value, use_ele_adc);

        sleep_ms(10);
    }

    return 0;
}



// 　角度出し用コード

// int main(void)
// {
//     stdio_init_all();
//     sleep_ms(2000); // USBシリアル安定待ち

//     printf("System start\n");

//     set_servo_dig_init();

//         while (true) {
//             set_servo_dig_controller();
//             sleep_ms(10);
//         }

//     return 0;
// }

// ADCリーダー用コード

// int main(void)
// {
//     stdio_init_all();
//     sleep_ms(2000); // USBシリアル安定待ち

//     printf("System start\n");

//     adc_reader();

//     return 0;
// }
