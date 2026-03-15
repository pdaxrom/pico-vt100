# ILI9486L LCD library for Raspberry Pi Pico

LCD display library for the ILI9486L controller. Includes an abstract display driver, baseline JPEG decoder, text rendering, and a VT100 terminal emulator.

The library architecture is split into:

- **`lcd_driver_t`** — abstract display interface with a vtable (concrete driver selected at link time)
- **ILI9486L** — hardware driver for Pico (SPI + DMA)
- **SDL2** — software driver for host-side development and testing
- **VT100 terminal** — runs on top of any driver via `lcd_driver_t`
- **JPEG** — baseline JPEG decoder, shared across all platforms

## Quick start

### Pico (ILI9486L)

```cmake
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)
project(my_app C CXX ASM)
pico_sdk_init()

set(ILI9486L_LCD_BUILD_DEMO OFF CACHE BOOL "" FORCE)
add_subdirectory(external/ili9486l_lcd)

add_executable(my_app src/main.c)
target_link_libraries(my_app PRIVATE ili9486l::lcd)
pico_add_extra_outputs(my_app)
```

```c
#include "lcd_driver.h"
#include "lcd_jpeg.h"
#include "vt100_terminal.h"

#include "pico/stdlib.h"

int main(void)
{
    lcd_driver_t *drv;

    stdio_init_all();
    drv = lcd_init();

    lcd_fill_screen(drv, LCD_COLOR_BLACK);

    while (true) {
        tight_loop_contents();
    }
}
```

Application code calls `lcd_init()` and receives an `lcd_driver_t *`. Which driver backs this pointer is determined at link time. Application code does not include driver-specific headers.

### Host (SDL2)

```bash
cmake -B build-host \
    -DILI9486L_LCD_HOST_BUILD=ON \
    -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build-host
```

The SDL2 window emulates a 480x320 display at 2x scale. The same `lcd_init()` call creates the SDL2 backend.

## Public headers

| Header | Purpose |
| --- | --- |
| `include/lcd_driver.h` | Abstract display interface, colors, `lcd_init()` / `lcd_destroy()` |
| `include/lcd_jpeg.h` | Baseline JPEG decoder |
| `include/vt100_terminal.h` | VT100/ANSI/VT52 terminal emulator |

Internal headers (`src/lcd_text.h`, `src/drivers/*/`) are not part of the public API.

## Display API

### Initialization and size

```c
lcd_driver_t *drv = lcd_init();       // initialize the driver (determined at link time)
lcd_destroy(drv);                      // release resources (SDL2) / no-op (Pico)

uint16_t w = lcd_width(drv);           // current width (480 with rotation=1)
uint16_t h = lcd_height(drv);          // current height (320 with rotation=1)
lcd_get_size(drv, &w, &h);            // same, in a single call
```

Default display size is `480x320` (`LCD_DISPLAY_WIDTH` / `LCD_DISPLAY_HEIGHT`). These macros can be overridden via `target_compile_definitions()`.

### Drawing primitives

```c
lcd_fill_screen(drv, LCD_COLOR_BLACK);
lcd_fill_rect(drv, x, y, w, h, LCD_COLOR_RED);
```

### Streaming path (no framebuffer)

```c
if (lcd_begin_write(drv, x, y, w, h)) {
    lcd_write_pixels(drv, rgb666_wire_data, pixel_count);  // may be async (DMA on Pico)
    lcd_flush(drv);                                         // wait for completion
}
```

### Colors

Internal format is `RGB666`. Each channel is 6 bits.

```c
lcd_color_t color = LCD_RGB666(0x3F, 0x00, 0x00);  // red (0x00..0x3F per channel)

// predefined:
LCD_COLOR_BLACK, LCD_COLOR_WHITE, LCD_COLOR_RED,
LCD_COLOR_GREEN, LCD_COLOR_BLUE, LCD_COLOR_CYAN,
LCD_COLOR_MAGENTA, LCD_COLOR_YELLOW
```

### Text rendering

Text rendering is available via the internal header `lcd_text.h` (located in `src/`):

```c
#include "lcd_text.h"

lcd_draw_char(drv, x, y, 'A', LCD_COLOR_WHITE, LCD_COLOR_BLACK, 2);
lcd_draw_string(drv, x, y, "HELLO", LCD_COLOR_WHITE, LCD_COLOR_BLACK, 2);
```

Built-in font is `5x7` pixels. The `scale` parameter sets the multiplier.

To use `lcd_text.h` from your application, add `src/` to the include path:

```cmake
target_include_directories(my_app PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/external/ili9486l_lcd/src
)
```

## JPEG

```c
#include "lcd_jpeg.h"

lcd_jpeg_info_t info;

if (lcd_jpeg_get_info(jpeg_data, jpeg_size, &info)) {
    // info.width, info.height
}

lcd_jpeg_draw(drv, jpeg_data, jpeg_size, x, y);
```

- Uses `TJpgDec`
- MCU-block decoding without a full-frame buffer
- Input data is read directly from flash / ROM / `const uint8_t[]`
- Baseline JPEG only (progressive is not supported)
- `lcd_jpeg_draw()` returns `false` if the image does not fit the display

## VT100 terminal

The terminal is designed for a `480x320` display:

- `80` columns, `35` rows
- cell size `6x9`, glyph size `5x7`

### Initialization

```c
#include "lcd_driver.h"
#include "vt100_terminal.h"

static vt100_terminal_t terminal;
lcd_driver_t *drv = lcd_init();

vt100_terminal_init(&terminal, drv, origin_x, origin_y);
```

`vt100_terminal_init()` immediately clears and draws the terminal area, so it must be called after `lcd_init()`.

### Data flow

Two data paths:

- **`vt100_terminal_putc()` / `vt100_terminal_write()`** — direct data path into the terminal parser (for a stream from a remote host)
- **`vt100_terminal_getch()`** — local user input path, passes through built-in commands and an optional hook before the terminal parser

### Terminal responses

If the host expects responses (`DA`, `DSR`):

```c
vt100_terminal_set_output(&terminal, my_output_fn, NULL);
```

### State update

```c
vt100_terminal_tick(&terminal, elapsed_ms);  // blink and housekeeping timers
```

### Built-in terminal UI

Activated via `vt100_terminal_getch()`:

- the last row is reserved for a status line
- `Ctrl+E` — enter local command mode
- `Ctrl+E 1/2/3` — switch to `80x34` / `80x30` / `80x24`
- `Ctrl+E s/p` — switch `SCROLL` / `PAGED` mode
- in `PAGED` mode, `XOFF`/`XON` are sent via the output callback

### Local hook

```c
static bool my_hook(vt100_terminal_t *terminal, char ch, void *user_data)
{
    if ((unsigned char)ch == 0x0C) {
        // handle locally
        return true;   // do not pass to parser
    }

    return false;      // pass through
}

vt100_terminal_set_getch_hook(&terminal, my_hook, NULL);
```

### Minimal terminal example

```c
#include "lcd_driver.h"
#include "vt100_terminal.h"

#include "pico/stdlib.h"

static vt100_terminal_t g_terminal;

static void terminal_output(const char *data, size_t len, void *user_data)
{
    (void)user_data;

    for (size_t i = 0; i < len; ++i) {
        putchar_raw((uint8_t)data[i]);
    }
}

int main(void)
{
    lcd_driver_t *drv;
    uint32_t last_ms;

    stdio_init_all();
    drv = lcd_init();

    vt100_terminal_init(&g_terminal, drv, 0u, 0u);
    vt100_terminal_set_output(&g_terminal, terminal_output, NULL);
    (void)vt100_terminal_getch(&g_terminal, PICO_ERROR_TIMEOUT);
    vt100_terminal_write(&g_terminal, "\x1b[2J\x1b[HREADY\r\n");

    last_ms = (uint32_t)to_ms_since_boot(get_absolute_time());

    while (true) {
        const int ch = getchar_timeout_us(0);
        const uint32_t now_ms = (uint32_t)to_ms_since_boot(get_absolute_time());

        vt100_terminal_tick(&g_terminal, now_ms - last_ms);
        last_ms = now_ms;

        (void)vt100_terminal_getch(&g_terminal, ch);

        tight_loop_contents();
    }
}
```

## Building

### Pico (standalone)

```bash
cmake -S . -B build
cmake --build build -j4
```

Artifacts: `build/demo/ili9486l_lcd_demo.elf`, `.uf2`

For Pico 2:

```bash
cmake -S . -B build-pico2 -DPICO_BOARD=pico2
cmake --build build-pico2 -j4
```

### Pico (submodule)

If the Pico SDK is already initialized in a parent project:

```cmake
set(ILI9486L_LCD_BUILD_DEMO OFF CACHE BOOL "" FORCE)
add_subdirectory(external/ili9486l_lcd)
target_link_libraries(my_app PRIVATE ili9486l::lcd)
```

### Host (SDL2)

```bash
cmake -B build-host \
    -DILI9486L_LCD_HOST_BUILD=ON \
    -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build-host
./build-host/demo/lcd_demo_sdl2
```

### Tests

```bash
cmake -S tests/host -B build-tests
cmake --build build-tests
ctest --test-dir build-tests --output-on-failure
```

Tests do not require the Pico SDK, SDL2, or hardware.

## Display wiring

Default pin assignments:

| Display | RP2040 |
| --- | --- |
| `CLK` | `GP14` |
| `MOSI` | `GP15` |
| `RES` | `GP11` |
| `DC` | `GP10` |
| `GND` | `GND` |
| `VCC` | `3V3(OUT)` |
| `BLK` | `3V3(OUT)` |

`CS` is not used — the module is assumed to be permanently selected on the SPI bus.

Pin and SPI overrides:

```cmake
target_compile_definitions(ili9486l_lcd PRIVATE
    LCD_SPI_PORT=spi0
    LCD_SPI_BAUDRATE_HZ=(40u*1000u*1000u)
    LCD_PIN_SCK=2
    LCD_PIN_MOSI=3
    LCD_PIN_RST=4
    LCD_PIN_DC=5
    LCD_PIN_BLK=6
)
```

## CMake options

| Option | Default | Purpose |
| --- | --- | --- |
| `ILI9486L_LCD_HOST_BUILD` | `OFF` | Force host build (without Pico SDK) |
| `ILI9486L_LCD_BUILD_DEMO` | `ON` (standalone) | Build the demo app |
| `ILI9486L_LCD_ENABLE_DEMO_STDIO_USB` | `ON` | USB CDC for demo |
| `ILI9486L_LCD_ENABLE_DEMO_STDIO_UART` | `ON` | UART stdio for demo |
| `ILI9486L_LCD_DEMO_RUN_FPS_TEST` | `ON` | Run benchmark at demo startup |
| `ILI9486L_LCD_DEMO_LOGO_PATH` | `demo/assets/logo.jpg` | JPEG path for boot logo |

## Preparing a boot logo

```bash
python3 -m pip install Pillow
python3 demo/tools/png_to_logo.py logo.png
python3 demo/tools/png_to_logo.py logo.png --rotate cw
```

Place the result in `demo/assets/logo.jpg`.

## Limitations

- The font API is not exported publicly: the built-in `5x7` font is used internally only
- UTF-8 and scrollback are not implemented in the terminal
- Baseline JPEG only
- Color order is configured as BGR (correct for the current display module)
