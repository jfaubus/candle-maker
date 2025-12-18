# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(candle-maker)



target_sources(app PRIVATE 
    src/cooling.c
    src/display.c
    src/heating.c
    src/stateMachine.c
    src/servos.c
    src/sensors.c
    src/motors.c

)