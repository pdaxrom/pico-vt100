# ILI9486L text demo for Raspberry Pi Pico

Минимальный проект под Raspberry Pi Pico / RP2040, который инициализирует дисплей `320x480` на контроллере `ILI9486L` по `4-wire SPI`, переводит его в `landscape` (`480x320`) и выводит текст шрифтом `5x7`.

## Подключение

Используемые сигналы дисплея:

| Дисплей | RP2040 |
| --- | --- |
| `CLK` | `GP14` |
| `MOS` | `GP15` |
| `RES` | `GP11` |
| `DC`  | `GP10` |
| `GND` | `GND` |
| `VCC` | `3V3(OUT)`* |
| `BLK` | `3V3(OUT)`** |

\* Подавайте питание так, как требует именно ваш модуль. Для чистого ILI9486L обычно нужен `3.3V`.

\** В коде управление подсветкой не используется, поэтому `BLK` проще всего подтянуть к `3V3(OUT)`. Если захотите управлять подсветкой с GPIO, достаточно назначить отдельный пин в `src/ili9486l.c`.

`CS` в вашем описании отсутствует, поэтому проект исходит из того, что модуль уже постоянно выбран на SPI-шине.

`MIS` (`MISO`) для этого примера не нужен и может оставаться неподключённым.

## Сборка

В `CMakeLists.txt` добавлен автоматический `include($HOME/cross-env.cmake)`. Если у вас уже есть:

```cmake
set(PICO_TOOLCHAIN_PATH "/path/to/arm-none-eabi")
set(PICO_SDK_PATH "/path/to/pico-sdk")
```

то отдельные переменные окружения задавать не нужно.

Сборка:

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
```

На выходе получится `ili9486l_lcd.uf2`.

## Конвертация логотипа

Для генерации [src/logo.h](/Users/sash/Work/RPI-PICO/ili9486l_lcd/src/logo.h) добавлен скрипт [tools/png_to_logo.py](/Users/sash/Work/RPI-PICO/ili9486l_lcd/tools/png_to_logo.py). Он конвертирует изображение в `rgb565le` так, как ожидает драйвер.

Нужна библиотека `Pillow`:

```bash
python3 -m pip install Pillow
```

Пример для уже готовой картинки `480x320`:

```bash
python3 tools/png_to_logo.py logo.png
```

Пример с поворотом в landscape и сохранением `raw` рядом:

```bash
python3 tools/png_to_logo.py logo.png --rotate cw --raw assets/logo.raw
```

Для совместимости также обновлён [assets/convert.sh](/Users/sash/Work/RPI-PICO/ili9486l_lcd/assets/convert.sh):

```bash
./assets/convert.sh logo.png
```

Если хотите обязательно прогонять картинку через `ImageMagick`, используйте [tools/logo_from_magick.sh](/Users/sash/Work/RPI-PICO/ili9486l_lcd/tools/logo_from_magick.sh):

```bash
bash tools/logo_from_magick.sh logo.png
bash tools/logo_from_magick.sh logo.png cw
```

## Что делает пример

- Инициализирует `spi1` на выводах `GP14/GP15`
- Аппаратно сбрасывает дисплей через `GP11`
- Переводит экран в `landscape` (`480x320`)
- Настраивает ILI9486L на `18-bit` режим SPI
- Показывает полноэкранный `RGB565` логотип 3 секунды
- Очищает экран и рисует несколько строк текста шрифтом `5x7`

## Ограничения

- Встроенный шрифт ориентирован на цифры, латиницу и базовую пунктуацию.
- Драйвер сейчас настроен на порядок цветов `BGR`, потому что на этом модуле при `RGB` красный и синий оказываются перепутаны. Если у вашего экземпляра цвета наоборот корректны только в `RGB`, уберите бит `BGR` в `ili9486l_set_rotation()` в файле `src/ili9486l.c`.
