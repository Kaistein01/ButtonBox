# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(Button_box C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(Button_box src/main.cpp
        src/hw_config.h
        src/config.hpp
        src/led_matrix.cpp
        src/led_matrix.hpp
        src/button_matrix.cpp
        src/button_matrix.hpp
        src/button_led_matrix.cpp
        src/button_led_matrix.hpp
        src/tft_display.cpp
        src/tft_display.hpp
        src/wifi_status.cpp
        src/wifi_status.hpp
        src/standalone_button.cpp
        src/standalone_button.hpp
        src/app_state.hpp
        src/api_client.cpp
        src/api_client.hpp
        src/app_controller.cpp
        src/app_controller.hpp
        src/config_store.cpp
        src/config_store.hpp
        src/webserver.cpp
        src/webserver.hpp
        ${PICO_SDK_PATH}/lib/tinyusb/lib/networking/dhserver.c
        )

pico_set_program_name(Button_box "Button_box")
pico_set_program_version(Button_box "0.1")

# Generate PIO header
pico_generate_pio_header(Button_box ${CMAKE_CURRENT_LIST_DIR}/blink.pio)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(Button_box 0)
pico_enable_stdio_usb(Button_box 1)

# Add the standard library to the build
target_link_libraries(Button_box
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(Button_box PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${PICO_SDK_PATH}/lib/tinyusb/lib/networking
)

# Add any user requested libraries
target_link_libraries(Button_box
        hardware_spi
        hardware_pio
        hardware_flash
        pico_cyw43_arch_lwip_threadsafe_background
        )

pico_add_extra_outputs(Button_box)

