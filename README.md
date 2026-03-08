# ILI9486L VT100 terminal for Raspberry Pi Pico

Минимальный проект под Raspberry Pi Pico / RP2040, который инициализирует дисплей `320x480` на контроллере `ILI9486L`, переводит его в `landscape` (`480x320`) и запускает на экране простой `VT100`-совместимый терминал `80x35` на шрифте `5x7`.

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

Boot logo теперь берётся напрямую из [assets/logo.jpg](/Users/sash/Work/RPI-PICO/ili9486l_lcd/assets/logo.jpg): файл линкуется в ELF как binary blob в `.rodata`, а на RP2040 декодируется встроенным `TJpgDec` небольшими блоками без полного framebuffer.

Внутренний пайплайн рисования теперь `RGB666`: обычная графика и терминал работают сразу в 6-битных каналах, а `JPEG` декодируется в `RGB888` и на выходе из декодера по месту сжимается до `RGB666` для отправки в дисплей.

## Терминальный режим

После boot logo прошивка очищает экран и запускает терминал `80x35`.

- размер ячейки: `6x9` пикселей
- экран: `480x320`, поэтому получается ровно `80` колонок и `35` строк
- glyph `5x7` рисуется с верхним и нижним пустым растром внутри ячейки
- вход: символы читаются из `stdio` и подаются в терминал по одному
- текущий `main` использует non-blocking `getchar_timeout_us(0)`, так что символы можно слать по UART или через USB CDC

Поддержанный subset VT100/ANSI:

- печатные ASCII-символы
- `CR`, `LF`, `BS`, `TAB`
- `ESC 7`, `ESC 8`, `ESC c`, `ESC D`, `ESC E`, `ESC M`, `ESC Z`
- `ESC #8` для `DECALN` (`screen alignment display`)
- `ESC H` для установки tab stop в текущей колонке
- `ESC ( 0`, `ESC ( B`, `ESC ( A`, `ESC ) 0`, `ESC ) B`, `ESC ) A`, `SO`, `SI` для `DEC Special Graphics` и `UK charset`
- `CSI A`, `B`, `C`, `D`, `E`, `F`, `G`, `H`, `I`, `S`, `T`, `Z`, `` ` ``, `a`, `b`, `d`, `e`, `f`
- `CSI J`, `K`, `L`, `M`, `@`, `P`, `X`, `r`, `n`, `c`, `g`, `h`, `l`
- `CSI m` для расширенного `SGR`: bold, faint, underline, blink flag, reverse, conceal, 16 цветов
- `CSI 4h` / `CSI 4l` для `IRM` (`insert/replace mode`)
- `CSI 20h` / `CSI 20l` для `LMN` (`new line mode` / `line feed mode`)
- `CSI ?5h` / `CSI ?5l` для `DECSCNM` (`reverse screen mode`)
- `CSI ?7h` / `CSI ?7l` для `DECAWM` (`autowrap`)
- `CSI ?6h` / `CSI ?6l` для `DECOM` (`origin mode`)
- `CSI ?25h` / `CSI ?25l` для показа/скрытия курсора
- `DA` / `DSR` ответы отправляются обратно через `stdio`
- tab stops по умолчанию стоят через каждые `8` колонок
- `CSI b` повторяет предыдущий печатный символ (`REP`)
- `CPR` (`CSI 6n`) учитывает `DECOM`: строка возвращается относительно scroll region, когда origin mode включён

Текущие требования:

- формат: `baseline JPEG` (`progressive JPEG` не поддерживается декодером)
- размер: ровно `480x320`
- ориентация: уже landscape или подготовленная скриптом

Для подготовки изображения оставлен скрипт [tools/png_to_logo.py](/Users/sash/Work/RPI-PICO/ili9486l_lcd/tools/png_to_logo.py). Он нормализует PNG/JPEG в baseline JPEG ассет.

Нужна библиотека `Pillow`:

```bash
python3 -m pip install Pillow
```

Пример для уже готовой картинки:

```bash
python3 tools/png_to_logo.py logo.png
```

Пример с поворотом в landscape:

```bash
python3 tools/png_to_logo.py logo.png --rotate cw
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
- Показывает встроенный в ELF `JPEG` логотип 3 секунды
- Декодирует логотип блоками прямо из flash без полного буфера кадра
- Запускает `VT100`-терминал `80x35`
- Принимает символы из `stdio` и рисует их на LCD по одному

## Ограничения

- Встроенный шрифт ориентирован на цифры, латиницу и базовую пунктуацию.
- Терминал не претендует на полный VT100. Реализован только базовый subset escape-последовательностей, который нужен для позиционирования курсора, очистки и простых цветов.
- `SGR blink` сейчас принимается и хранится как атрибут, но без таймера анимации мигания.
- `DEC Special Graphics` сейчас в первую очередь покрывает line-drawing символы для рамок и пересечений.
- В терминальном режиме сейчас нет аппаратного scrollback и нет поддержки UTF-8.
- Декодер поддерживает только `baseline JPEG`. Если сохранить логотип как `progressive JPEG`, boot logo не покажется.
- Логотип сейчас ожидается размером ровно `480x320`, иначе код уйдёт в fallback с чёрным экраном перед следующими демо-экранами.
- Драйвер сейчас настроен на порядок цветов `BGR`, потому что на этом модуле при `RGB` красный и синий оказываются перепутаны. Если у вашего экземпляра цвета наоборот корректны только в `RGB`, уберите бит `BGR` в `ili9486l_set_rotation()` в файле `src/ili9486l.c`.
