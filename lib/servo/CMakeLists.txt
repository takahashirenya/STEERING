# lib/SERVO/CMakeLists.txt
add_library(servo STATIC
    servo.c
    servo_adc.c
    servo_pwm.c
)

# このライブラリを使う人にヘッダを公開
target_include_directories(servo PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

# ドライバ内で I2C を使うなら（pico-sdk の i2c API）
target_link_libraries(servo
    hardware_pwm
    hardware_adc
    pico_stdlib    
)