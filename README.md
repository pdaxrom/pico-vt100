# ILI9486L LCD library for Raspberry Pi Pico

Библиотека для работы с LCD-дисплеем на контроллере ILI9486L. Включает абстрактный драйвер дисплея, baseline JPEG декодер, текстовый вывод и VT100-терминал.

Архитектура библиотеки разделена на:

- **`lcd_driver_t`** — абстрактный интерфейс дисплея с vtable (подключение конкретного драйвера через линковку)
- **ILI9486L** — аппаратный драйвер для Pico (SPI + DMA)
- **SDL2** — программный драйвер для разработки и тестирования на хосте
- **VT100 терминал** — работает поверх любого драйвера через `lcd_driver_t`
- **JPEG** — baseline JPEG декодер, общий для всех платформ

## Быстрый старт

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

Приложение вызывает `lcd_init()` и получает `lcd_driver_t *`. Какой именно драйвер стоит за этим указателем — определяется на этапе линковки. Прикладной код не включает заголовки конкретного драйвера.

### Host (SDL2)

```bash
cmake -B build-host \
    -DILI9486L_LCD_HOST_BUILD=ON \
    -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build-host
```

SDL2 окно эмулирует дисплей 480x320 с масштабом 2x. Тот же вызов `lcd_init()` создаёт SDL2-бэкенд.

## Публичные заголовки

| Заголовок | Назначение |
| --- | --- |
| `include/lcd_driver.h` | Абстрактный интерфейс дисплея, цвета, `lcd_init()` / `lcd_destroy()` |
| `include/lcd_jpeg.h` | Baseline JPEG декодер |
| `include/vt100_terminal.h` | VT100/ANSI/VT52 терминал |

Внутренние заголовки (`src/lcd_text.h`, `src/drivers/*/`) не являются частью публичного API.

## API дисплея

### Инициализация и размер

```c
lcd_driver_t *drv = lcd_init();       // инициализировать драйвер (определяется линковкой)
lcd_destroy(drv);                      // освободить ресурсы (SDL2) / no-op (Pico)

uint16_t w = lcd_width(drv);           // текущая ширина (480 после rotation=1)
uint16_t h = lcd_height(drv);          // текущая высота (320 после rotation=1)
lcd_get_size(drv, &w, &h);            // то же, одним вызовом
```

Размер дисплея по умолчанию `480x320` (`LCD_DISPLAY_WIDTH` / `LCD_DISPLAY_HEIGHT`). Эти макросы можно переопределить через `target_compile_definitions()`.

### Примитивы рисования

```c
lcd_fill_screen(drv, LCD_COLOR_BLACK);
lcd_fill_rect(drv, x, y, w, h, LCD_COLOR_RED);
```

### Streaming path (без framebuffer)

```c
if (lcd_begin_write(drv, x, y, w, h)) {
    lcd_write_pixels(drv, rgb666_wire_data, pixel_count);  // может быть async (DMA на Pico)
    lcd_flush(drv);                                         // дождаться завершения
}
```

### Цвета

Внутренний формат — `RGB666`. Каждый канал 6 бит.

```c
lcd_color_t color = LCD_RGB666(0x3F, 0x00, 0x00);  // красный (0x00..0x3F на канал)

// предопределённые:
LCD_COLOR_BLACK, LCD_COLOR_WHITE, LCD_COLOR_RED,
LCD_COLOR_GREEN, LCD_COLOR_BLUE, LCD_COLOR_CYAN,
LCD_COLOR_MAGENTA, LCD_COLOR_YELLOW
```

### Текстовый вывод

Текстовый вывод доступен через внутренний заголовок `lcd_text.h` (подключается из `src/`):

```c
#include "lcd_text.h"

lcd_draw_char(drv, x, y, 'A', LCD_COLOR_WHITE, LCD_COLOR_BLACK, 2);
lcd_draw_string(drv, x, y, "HELLO", LCD_COLOR_WHITE, LCD_COLOR_BLACK, 2);
```

Встроенный шрифт — `5x7` пикселей. Параметр `scale` задаёт масштаб.

Для использования `lcd_text.h` из своего приложения нужно добавить `src/` в include path:

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

- Используется `TJpgDec`
- Декодирование MCU-блоками без full-frame буфера
- Входные данные читаются прямо из flash / ROM / `const uint8_t[]`
- Только baseline JPEG (progressive не поддерживается)
- `lcd_jpeg_draw()` вернёт `false`, если изображение не помещается в дисплей

## VT100 терминал

Терминал рассчитан на дисплей `480x320`:

- `80` колонок, `35` строк
- ячейка `6x9`, глиф `5x7`

### Инициализация

```c
#include "lcd_driver.h"
#include "vt100_terminal.h"

static vt100_terminal_t terminal;
lcd_driver_t *drv = lcd_init();

vt100_terminal_init(&terminal, drv, origin_x, origin_y);
```

`vt100_terminal_init()` сразу очищает и рисует область терминала, поэтому вызывать его нужно после `lcd_init()`.

### Потоки данных

Два пути подачи данных:

- **`vt100_terminal_putc()` / `vt100_terminal_write()`** — прямой путь данных в terminal parser (для потока от удалённого хоста)
- **`vt100_terminal_getch()`** — путь локального пользовательского ввода, проходит через встроенные команды и optional hook перед terminal parser

### Ответы терминала

Если хост ожидает ответы (`DA`, `DSR`):

```c
vt100_terminal_set_output(&terminal, my_output_fn, NULL);
```

### Обновление состояния

```c
vt100_terminal_tick(&terminal, elapsed_ms);  // blink и служебные таймеры
```

### Встроенная terminal UI

Активируется через `vt100_terminal_getch()`:

- последняя строка резервируется под status line
- `Ctrl+E` — войти в режим локальной команды
- `Ctrl+E 1/2/3` — переключить `80x34` / `80x30` / `80x24`
- `Ctrl+E s/p` — переключить `SCROLL` / `PAGED`
- в `PAGED` режиме отправляются `XOFF`/`XON` через output callback

### Локальный hook

```c
static bool my_hook(vt100_terminal_t *terminal, char ch, void *user_data)
{
    if ((unsigned char)ch == 0x0C) {
        // обработать локально
        return true;   // не передавать в parser
    }

    return false;      // передать дальше
}

vt100_terminal_set_getch_hook(&terminal, my_hook, NULL);
```

### Минимальный пример терминала

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

## Сборка

### Pico (standalone)

```bash
cmake -S . -B build
cmake --build build -j4
```

Артефакты: `build/demo/ili9486l_lcd_demo.elf`, `.uf2`

Для Pico 2:

```bash
cmake -S . -B build-pico2 -DPICO_BOARD=pico2
cmake --build build-pico2 -j4
```

### Pico (submodule)

Если Pico SDK уже инициализирован в родительском проекте:

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

### Тесты

```bash
cmake -S tests/host -B build-tests
cmake --build build-tests
ctest --test-dir build-tests --output-on-failure
```

Тесты не требуют Pico SDK, SDL2 или железа.

## Подключение дисплея

Пины по умолчанию:

| Дисплей | RP2040 |
| --- | --- |
| `CLK` | `GP14` |
| `MOSI` | `GP15` |
| `RES` | `GP11` |
| `DC` | `GP10` |
| `GND` | `GND` |
| `VCC` | `3V3(OUT)` |
| `BLK` | `3V3(OUT)` |

`CS` не используется — предполагается, что модуль постоянно выбран на SPI-шине.

Переопределение пинов и SPI:

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

## CMake-опции

| Опция | Умолчание | Назначение |
| --- | --- | --- |
| `ILI9486L_LCD_HOST_BUILD` | `OFF` | Форсировать host-сборку (без Pico SDK) |
| `ILI9486L_LCD_BUILD_DEMO` | `ON` (standalone) | Собрать demo app |
| `ILI9486L_LCD_ENABLE_DEMO_STDIO_USB` | `ON` | USB CDC для demo |
| `ILI9486L_LCD_ENABLE_DEMO_STDIO_UART` | `ON` | UART stdio для demo |
| `ILI9486L_LCD_DEMO_RUN_FPS_TEST` | `ON` | Benchmark при старте demo |
| `ILI9486L_LCD_DEMO_LOGO_PATH` | `demo/assets/logo.jpg` | Путь к JPEG для boot logo |

## Подготовка boot logo

```bash
python3 -m pip install Pillow
python3 demo/tools/png_to_logo.py logo.png
python3 demo/tools/png_to_logo.py logo.png --rotate cw
```

Результат положить в `demo/assets/logo.jpg`.

## Ограничения

- Публичный шрифтовой API не экспортируется: встроенный `5x7` используется только внутренне
- UTF-8 и scrollback в терминале не реализованы
- Только baseline JPEG
- Порядок цветов настроен как BGR (корректен для текущего модуля)
