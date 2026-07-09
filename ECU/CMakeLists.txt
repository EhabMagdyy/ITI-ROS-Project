cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ecu)

target_include_directories(app PRIVATE inc)

target_sources(app PRIVATE
    src/main.c
    src/motor.c
    src/encoder.c
    src/ultrasonic.c
    src/uart_gateway.c
    src/imu.c
)
