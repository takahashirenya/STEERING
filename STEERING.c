#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/uart.h"
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
#define CAN_ID_AIRSPEED 0x004
#define CAN_TIMEOUT_MS  100U

// UART used to exchange data with LOGGER.
#define LOGGER_UART        uart0
#define LOGGER_UART_TX_PIN 16
#define LOGGER_UART_RX_PIN 17
#define LOGGER_UART_BAUD   115200
#define SPEED_FRAME_SIZE   8U
#define SPEED_DATA_SIZE    ((uint8_t)sizeof(float))
#define PWM_FRAME_HEADER_1 0xA5U
#define PWM_FRAME_HEADER_2 0x5AU
#define PWM_DATA_SIZE      5U
#define PWM_FRAME_SIZE     9U


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

static uint8_t speed_frame[SPEED_FRAME_SIZE];
static uint8_t speed_frame_index = 0;


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

static void logger_uart_init(void)
{
    uart_init(LOGGER_UART, LOGGER_UART_BAUD);
    uart_set_format(LOGGER_UART, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(LOGGER_UART, true);
    gpio_set_function(LOGGER_UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(LOGGER_UART_RX_PIN, GPIO_FUNC_UART);
}

static void send_airspeed_can(mcp2515_t *can, const uint8_t *speed_data)
{
    float speed_mps;
    memcpy(&speed_mps, speed_data, sizeof(speed_mps));
    printf("UART RX Data=[%02X %02X %02X %02X], airspeed=%.6f m/s\n",
           speed_data[0], speed_data[1], speed_data[2], speed_data[3],
           speed_mps);

    can_frame_t frame = {
        .id = CAN_ID_AIRSPEED,
        .dlc = SPEED_DATA_SIZE,
        .extended = false,
        .rtr = false,
    };
    memcpy(frame.data, speed_data, SPEED_DATA_SIZE);
    mcp2515_send_message(can, &frame);
}

static void logger_uart_to_can_process(mcp2515_t *can)
{
    while (uart_is_readable(LOGGER_UART)) {
        uint8_t byte = uart_getc(LOGGER_UART);

        if (speed_frame_index == 0 && byte != 0xAA) {
            continue;
        }
        if (speed_frame_index == 1 && byte != 0x55) {
            speed_frame_index = (byte == 0xAA) ? 1 : 0;
            continue;
        }
        if (speed_frame_index == 2 && byte != SPEED_DATA_SIZE) {
            speed_frame_index = (byte == 0xAA) ? 1 : 0;
            continue;
        }

        speed_frame[speed_frame_index++] = byte;
        if (speed_frame_index == SPEED_FRAME_SIZE) {
            uint8_t checksum = 0;
            for (uint8_t i = 0; i < SPEED_FRAME_SIZE - 1; i++) {
                checksum ^= speed_frame[i];
            }
            if (checksum == speed_frame[SPEED_FRAME_SIZE - 1]) {
                send_airspeed_can(can, &speed_frame[3]);
            }
            speed_frame_index = 0;
        }
    }
}

static void send_pwm_to_logger(uint16_t lad_pwm_us, uint16_t ele_pwm_us,
                               hatch_gear_t hatch_gear)
{
    uint8_t frame[PWM_FRAME_SIZE] = {
        PWM_FRAME_HEADER_1, PWM_FRAME_HEADER_2, PWM_DATA_SIZE,
        (uint8_t)(lad_pwm_us & 0xFFU), (uint8_t)(lad_pwm_us >> 8),
        (uint8_t)(ele_pwm_us & 0xFFU), (uint8_t)(ele_pwm_us >> 8),
        (uint8_t)hatch_gear,
        0U,
    };
    for (uint8_t i = 0; i < PWM_FRAME_SIZE - 1; i++) {
        frame[PWM_FRAME_SIZE - 1] ^= frame[i];
    }
    uart_write_blocking(LOGGER_UART, frame, sizeof(frame));
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

    logger_uart_init();

    // ボタン状態管理初期化
    button_state_init();

    // ハッチ・ギアのタイマー初期化
    hatch_gear_timer_init();

    // 尾翼サーボ初期化
    tail_setup();

    hatch_gear_pwm_init();
    uint32_t last_pwm_tx_ms = 0;

    while (true) {
        uint32_t now = now_ms();

        // CAN読み取り
        can_read_process(&can, now);

        // Forward each valid LOGGER UART airspeed frame on standard CAN ID 0x004.
        logger_uart_to_can_process(&can);

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
        // CAN button data: data[0] == 0 is released, nonzero is pressed.
        bool use_button_can = can_data_is_fresh(can_button_received, can_button_last_ms, now) && can_button_dlc >= 1;
        bool button_now = use_button_can ? (can_button_data[0] != 0) : false;

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
        tail_pwm_values_t tail_pwm = tail_controller_with_adc_values(lad_adc_value, use_lad_adc, ele_adc_value, use_ele_adc);
        if (now - last_pwm_tx_ms >= 20U) {
            send_pwm_to_logger(tail_pwm.lad, tail_pwm.ele, hatch_gear_state_get());
            last_pwm_tx_ms = now;
        }
        printf("lad_pwm: %u, ele_pwm: %u\n", tail_pwm.lad, tail_pwm.ele);

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
