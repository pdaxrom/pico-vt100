cmake_minimum_required(VERSION 3.13)

# Check for custom cross environment settings
if(EXISTS $ENV{HOME}/cross-env.cmake)
  message(STATUS "Custom cross environment detected")
  include($ENV{HOME}/cross-env.cmake)
else()
  message(STATUS "Custom cross environment is not detected")
endif()

if(NOT DEFINED PICO_SDK_PATH AND DEFINED ENV{PICO_SDK_PATH})
  set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
endif()

if(NOT DEFINED PICO_SDK_PATH)
  message(FATAL_ERROR "PICO_SDK_PATH is not set. Configure it in $ENV{HOME}/cross-env.cmake or export it in the environment.")
endif()

include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(ili9486l_lcd C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

pico_sdk_init()

add_executable(ili9486l_lcd
  src/font5x7.c
  src/ili9486l.c
  src/jpeg_logo.c
  src/logo_jpg.S
  src/main.c
  src/tjpgd/tjpgd.c
)

target_include_directories(ili9486l_lcd PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/src
)

target_compile_definitions(ili9486l_lcd PRIVATE
  LOGO_JPG_PATH=\"${CMAKE_CURRENT_LIST_DIR}/assets/logo.jpg\"
)

target_link_libraries(ili9486l_lcd PRIVATE
  pico_stdlib
  hardware_spi
)

pico_enable_stdio_usb(ili9486l_lcd 0)
pico_enable_stdio_uart(ili9486l_lcd 1)

pico_set_program_name(ili9486l_lcd "ili9486l_lcd")
pico_set_program_version(ili9486l_lcd "1.0")

pico_add_extra_outputs(ili9486l_lcd)
