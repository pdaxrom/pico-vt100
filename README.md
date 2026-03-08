# ILI9486L LCD library and demo for Raspberry Pi Pico

Проект теперь разделён на библиотеку и демонстрационное приложение:

- `ili9486l_lcd` - статическая библиотека для дисплея, baseline JPEG и VT100-терминала
- `ili9486l::lcd` - alias target для `target_link_libraries(...)`
- `ili9486l_lcd_demo` - demo app из `src/main.c`

Публичные заголовки лежат в `include/`:

- `include/ili9486l.h`
- `include/ili9486l_jpeg.h`
- `include/vt100_terminal.h`

Внутренние файлы вроде `font5x7.*` и `tjpgd/*` остаются приватными деталями реализации.

## Подключение дисплея

Используемые сигналы в demo:

| Дисплей | RP2040 |
| --- | --- |
| `CLK` | `GP14` |
| `MOSI` | `GP15` |
| `RES` | `GP11` |
| `DC` | `GP10` |
| `GND` | `GND` |
| `VCC` | `3V3(OUT)`* |
| `BLK` | `3V3(OUT)`** |

\* Для чистого ILI9486L обычно нужен `3.3V`, но проверьте требования своего модуля.

\** В demo управление подсветкой не используется. Если нужно управлять ей с GPIO, достаточно поменять конфиг в `src/ili9486l.c`.

`CS` в проекте не используется: предполагается, что модуль уже постоянно выбран на SPI-шине.

## Standalone build

Для standalone-сборки проект по-прежнему умеет подтягивать `$HOME/cross-env.cmake`. Если `PICO_SDK_PATH` не передан из родительского проекта, он берётся из:

- `$HOME/cross-env.cmake`
- или переменной окружения `PICO_SDK_PATH`

Сборка demo:

```bash
cmake -S . -B build
cmake --build build -j4
```

Если каталог `build/` был сконфигурирован старой версией `CMakeLists.txt` и закэшировал host compiler (`/usr/bin/cc`), его нужно пересоздать или использовать новый build directory.

Артефакты demo:

- `build/ili9486l_lcd_demo.elf`
- `build/ili9486l_lcd_demo.uf2`

Для `pico2`:

```bash
cmake -S . -B build-pico2 -DPICO_BOARD=pico2
cmake --build build-pico2 -j4
```

## Использование как submodule

Если Pico SDK уже импортирован и инициализирован в родительском проекте, можно просто делать `add_subdirectory(...)`.

В родительском проекте порядок должен быть стандартный для Pico SDK:

1. определить `PICO_SDK_PATH`
2. сделать `include(.../pico_sdk_import.cmake)`
3. вызвать `project(...)`
4. вызвать `pico_sdk_init()`
5. подключить этот проект через `add_subdirectory(...)`

Пример:

```cmake
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)
project(my_app C CXX ASM)
pico_sdk_init()

set(ILI9486L_LCD_BUILD_DEMO OFF CACHE BOOL "" FORCE)
add_subdirectory(external/ili9486l_lcd)

target_link_libraries(my_app PRIVATE ili9486l::lcd)
```

Если demo в родительском проекте не нужен, оставляйте:

```cmake
set(ILI9486L_LCD_BUILD_DEMO OFF CACHE BOOL "" FORCE)
```

Полезные опции:

- `ILI9486L_LCD_BUILD_DEMO` - собрать `src/main.c` как demo app
- `ILI9486L_LCD_ENABLE_DEMO_STDIO_USB` - включить USB CDC для demo
- `ILI9486L_LCD_ENABLE_DEMO_STDIO_UART` - включить UART stdio для demo

## Работа с дисплеем

Публичный API: `include/ili9486l.h`

Что даёт библиотека:

- инициализацию ILI9486L в `landscape` режиме `480x320`
- базовые примитивы: `draw_pixel`, `fill_rect`, `fill_screen`, `draw_char`, `draw_string`
- streaming path без framebuffer через:
  - `ili9486l_begin_write()`
  - `ili9486l_write_rgb666_pixels()`
  - `ili9486l_write_rgb666_wire_pixels()`
  - `ili9486l_write_rgb888_as_rgb666_pixels()`
- прямой вывод прямоугольников в `RGB666`
- аппаратный vertical scroll (`VSCRDEF` / `VSCRSADD`)

Внутренний формат библиотеки - `RGB666`.
Если входящие данные `RGB888`, библиотека сжимает их до `RGB666` при отправке в дисплей.

Горячий путь вывода оптимизирован под low-RAM режим:

- без полного framebuffer
- с DMA для крупных SPI-передач
- с raw on-wire path для уже упакованных `RGB666` данных

## Работа с JPEG

Публичный API: `include/ili9486l_jpeg.h`

Доступные функции:

```c
bool ili9486l_jpeg_get_info(const uint8_t *jpeg_data, size_t jpeg_size, ili9486l_jpeg_info_t *out_info);
bool ili9486l_jpeg_draw(const uint8_t *jpeg_data, size_t jpeg_size, uint16_t x, uint16_t y);
```

Свойства JPEG-пути:

- используется `TJpgDec`
- декодирование идёт MCU-блоками без full-frame буфера
- входные данные читаются прямо из flash / ROM / `const uint8_t[]`
- декодер выдаёт `RGB888`, библиотека сразу пишет это в дисплей как `RGB666`

Ограничения JPEG:

- только `baseline JPEG`
- `progressive JPEG` не поддерживается
- `ili9486l_jpeg_draw()` вернёт `false`, если изображение не помещается в текущую геометрию дисплея

Demo app использует этот API для boot logo, а сам JPEG-файл `assets/logo.jpg` линкуется в ELF через `.incbin` в `src/logo_jpg.S`.

## VT100 терминал

Публичный API: `include/vt100_terminal.h`

Терминал рассчитан на дисплей `480x320`:

- `80` колонок
- `35` строк
- ячейка `6x9`
- глиф `5x7` с верхним и нижним зазором

Основные функции:

```c
void vt100_terminal_init(vt100_terminal_t *terminal, uint16_t origin_x, uint16_t origin_y);
void vt100_terminal_set_output(vt100_terminal_t *terminal, vt100_terminal_output_fn output_fn, void *user_data);
void vt100_terminal_putc(vt100_terminal_t *terminal, char ch);
void vt100_terminal_write(vt100_terminal_t *terminal, const char *text);
void vt100_terminal_tick(vt100_terminal_t *terminal, uint32_t elapsed_ms);
void vt100_terminal_render(vt100_terminal_t *terminal);
```

Что реализовано:

- потоковый приём символов по одному байту
- символьный screen buffer без framebuffer дисплея
- subset `VT100`, `ANSI`, `DEC Special Graphics`, `UK charset`, `VT52`
- `SGR` цвета и атрибуты
- `DECOM`, `DECAWM`, `DECSCNM`, `IRM`, `LMN`, scroll region, tab stops
- output callback для `DA` / `DSR` ответов обратно хосту
- blinking attribute через `vt100_terminal_tick(elapsed_ms)`

Терминал не претендует на полный VT100. Актуальные пробелы перечислены в `TODO.md`.

## Demo app

`src/main.c` теперь считается именно демонстрационным приложением, а не "центром" проекта.

Что делает demo:

1. Инициализирует дисплей.
2. Показывает встроенный `assets/logo.jpg`.
3. Запускает терминал `80x35`.
4. Читает символы из `stdio`.
5. Обновляет blink через `vt100_terminal_tick()`.

Demo использует и USB CDC, и UART stdio по умолчанию. Это можно отключить CMake-опциями `ILI9486L_LCD_ENABLE_DEMO_STDIO_USB` и `ILI9486L_LCD_ENABLE_DEMO_STDIO_UART`.

## Подготовка boot logo

Подготовка JPEG оставлена как отдельный pipeline через:

- `tools/png_to_logo.py`
- `tools/logo_from_magick.sh`
- `assets/convert.sh`

Нужна `Pillow`:

```bash
python3 -m pip install Pillow
```

Пример:

```bash
python3 tools/png_to_logo.py logo.png
python3 tools/png_to_logo.py logo.png --rotate cw
```

Итоговый файл нужно положить в `assets/logo.jpg`. Demo подхватывает его на этапе линковки.

## Host tests

Есть отдельные native tests для VT100-парсера:

```bash
cmake -S tests/host -B /tmp/ili9486l_lcd-build-host
cmake --build /tmp/ili9486l_lcd-build-host -j4
ctest --test-dir /tmp/ili9486l_lcd-build-host --output-on-failure
```

Они не требуют Pico SDK и не требуют железа.

## Ограничения

- библиотека ориентирована на один конкретный контроллер и текущий конфиг SPI в `src/ili9486l.c`
- публичный шрифтовой API не экспортируется: встроенный `5x7` используется только внутренне
- UTF-8 и scrollback в терминале пока не реализованы
- `DEC Special Graphics` покрывает практичный subset, а не весь исторический терминальный стек
- порядок цветов сейчас настроен как `BGR`, потому что на текущем модуле это даёт корректные цвета
