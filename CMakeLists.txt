cmake_minimum_required(VERSION 3.13)

add_compile_definitions(PICO_DEFAULT_UART_BAUD_RATE=921600)

set(PICO_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/pico-sdk")
set(PICO_TINYUSB_PATH "${CMAKE_CURRENT_LIST_DIR}/tinyusb")

include(pico_sdk_import.cmake)

project(spaceball)

pico_sdk_init()

add_compile_options(-Wall)

add_executable(spaceball
    src/spaceball.c
    src/descriptors.c
)

target_include_directories(spaceball PRIVATE src)

target_link_libraries(spaceball
    pico_stdlib
    tinyusb_device
    tinyusb_board
)

pico_add_extra_outputs(spaceball)
