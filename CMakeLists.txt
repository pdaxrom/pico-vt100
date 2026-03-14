cmake_minimum_required(VERSION 3.13)

set(ILI9486L_LCD_IS_TOP_LEVEL OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(ILI9486L_LCD_IS_TOP_LEVEL ON)
endif()

# Detect whether the Pico SDK is available. When it is not, we are doing a
# host-only build (SDL2 demo + tests).
# Pass -DILI9486L_LCD_HOST_BUILD=ON to force a host build even when the SDK
# is reachable.
option(ILI9486L_LCD_HOST_BUILD "Force a host (non-Pico) build" OFF)

set(ILI9486L_LCD_PICO_BUILD OFF)

if(NOT ILI9486L_LCD_HOST_BUILD)
  if(COMMAND pico_sdk_init)
    set(ILI9486L_LCD_PICO_BUILD ON)
  else()
    if(ILI9486L_LCD_IS_TOP_LEVEL)
      if(EXISTS $ENV{HOME}/cross-env.cmake)
        message(STATUS "Custom cross environment detected")
        include($ENV{HOME}/cross-env.cmake)
      else()
        message(STATUS "Custom cross environment is not detected")
      endif()
    endif()

    if(NOT DEFINED PICO_SDK_PATH AND DEFINED ENV{PICO_SDK_PATH})
      set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
    endif()

    if(DEFINED PICO_SDK_PATH)
      include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)
      set(ILI9486L_LCD_PICO_BUILD ON)
    endif()
  endif()
endif()

project(ili9486l_lcd C CXX ASM)

option(ILI9486L_LCD_BUILD_DEMO "Build the demo application from demo/" ${ILI9486L_LCD_IS_TOP_LEVEL})
option(ILI9486L_LCD_ENABLE_DEMO_STDIO_USB "Enable USB stdio for the demo application" ON)
option(ILI9486L_LCD_ENABLE_DEMO_STDIO_UART "Enable UART stdio for the demo application" ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Common sources (shared between Pico and host builds) ---
set(ILI9486L_LCD_COMMON_SOURCES
  src/font5x7.c
  src/font5x7_row_masks.c
  src/lcd_text.c
  src/lcd_jpeg.c
  src/tjpgd/tjpgd.c
  src/vt100_terminal.c
  src/vt100_terminal_console.c
)

if(ILI9486L_LCD_PICO_BUILD)
  # Pico SDK for RP2350 uses GNU C constructs such as `asm` in C sources.
  # Keep GNU extensions enabled so both RP2040 and Pico 2 builds work.
  set(CMAKE_C_EXTENSIONS ON)

  if(NOT TARGET pico_stdlib)
    pico_sdk_init()
  endif()

  set(ILI9486L_LCD_PUBLIC_HEADERS
    include/lcd_driver.h
    include/lcd_jpeg.h
    include/vt100_terminal.h
  )

  set(ILI9486L_LCD_LIBRARY_SOURCES
    ${ILI9486L_LCD_COMMON_SOURCES}
    src/drivers/ili9486l/ili9486l.c
  )

  add_library(ili9486l_lcd STATIC
    ${ILI9486L_LCD_PUBLIC_HEADERS}
    ${ILI9486L_LCD_LIBRARY_SOURCES}
  )

  add_library(ili9486l::lcd ALIAS ili9486l_lcd)

  target_include_directories(ili9486l_lcd
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
    PRIVATE
      ${CMAKE_CURRENT_LIST_DIR}/src
  )

  target_link_libraries(ili9486l_lcd
    PUBLIC
      pico_stdlib
      hardware_dma
      hardware_spi
  )

  if(ILI9486L_LCD_BUILD_DEMO)
    add_subdirectory(demo)
  endif()

else()
  # --- Host build (SDL2 backend) ---
  set(CMAKE_C_EXTENSIONS OFF)

  find_package(SDL2 QUIET)

  # Common host library (no driver — tests provide their own stub)
  add_library(ili9486l_lcd_common STATIC
    ${ILI9486L_LCD_COMMON_SOURCES}
  )

  target_include_directories(ili9486l_lcd_common
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
    PRIVATE
      ${CMAKE_CURRENT_LIST_DIR}/src
  )

  if(SDL2_FOUND)
    add_library(sdl2_lcd STATIC
      src/drivers/sdl2/sdl2_lcd.c
    )

    target_include_directories(sdl2_lcd
      PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src>
    )

    target_link_libraries(sdl2_lcd
      PUBLIC
        ili9486l_lcd_common
        SDL2::SDL2
    )

    if(ILI9486L_LCD_BUILD_DEMO)
      add_subdirectory(demo)
    endif()
  else()
    message(STATUS "SDL2 not found — skipping SDL2 driver and host demo")
  endif()
endif()
