cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(PICO_BOARD pico_w CACHE STRING "Board type")

include(pico_sdk_import.cmake)  

project(sinalizacao_hospitalar C CXX ASM) 

pico_sdk_init()

add_executable(${PROJECT_NAME}
    sinalizacao_hospitalar.c
    ssd1306.c
)

pico_set_program_name(${PROJECT_NAME} "sinalizacao_hospitalar")
pico_set_program_version(${PROJECT_NAME} "0.1")

pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)

pico_enable_stdio_uart(${PROJECT_NAME} 0)
pico_enable_stdio_usb(${PROJECT_NAME} 1)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    hardware_i2c
    hardware_pio
    hardware_adc
    hardware_pwm
)

pico_add_extra_outputs(${PROJECT_NAME})