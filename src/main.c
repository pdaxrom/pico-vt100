#include "ili9486l.h"
#include "jpeg_logo.h"

#include "pico/stdlib.h"

static void show_boot_logo(void) {
  if (!jpeg_logo_show()) {
    ili9486l_fill_screen(LCD_COLOR_BLACK);
    return;
  }

  sleep_ms(3000);
}

static void draw_rgb_gradient_strip(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
  static const uint8_t stops[][3] = {
    {63u, 0u, 0u},
    {63u, 32u, 0u},
    {63u, 63u, 0u},
    {0u, 63u, 0u},
    {0u, 63u, 63u},
    {0u, 0u, 63u},
    {44u, 0u, 63u},
  };
  const uint16_t max_phase = (uint16_t)((sizeof(stops) / sizeof(stops[0]) - 1u) * 255u);

  if (w == 0 || h == 0) {
    return;
  }

  for (uint16_t i = 0; i < w; ++i) {
    uint16_t segment = 0;
    uint16_t mix = 0;
    uint8_t red;
    uint8_t green;
    uint8_t blue;

    if (w > 1) {
      const uint16_t phase = (uint16_t)(((uint32_t)i * max_phase) / (uint32_t)(w - 1u));

      segment = (uint16_t)(phase / 255u);
      mix = (uint16_t)(phase % 255u);

      if (segment >= (sizeof(stops) / sizeof(stops[0]) - 1u)) {
        segment = (uint16_t)(sizeof(stops) / sizeof(stops[0]) - 2u);
        mix = 255u;
      }
    }

    red = (uint8_t)(((uint16_t)stops[segment][0] * (255u - mix) + (uint16_t)stops[segment + 1u][0] * mix) / 255u);
    green = (uint8_t)(((uint16_t)stops[segment][1] * (255u - mix) + (uint16_t)stops[segment + 1u][1] * mix) / 255u);
    blue = (uint8_t)(((uint16_t)stops[segment][2] * (255u - mix) + (uint16_t)stops[segment + 1u][2] * mix) / 255u);

    ili9486l_fill_rect_rgb666((uint16_t)(x + i), y, 1, h, red, green, blue);
  }
}

static void draw_channel_gradient_strip(uint16_t x, uint16_t y, uint16_t w, uint16_t h, char channel) {
  if (w == 0 || h == 0) {
    return;
  }

  for (uint16_t i = 0; i < w; ++i) {
    uint8_t level = 0;
    uint8_t r6 = 0;
    uint8_t g6 = 0;
    uint8_t b6 = 0;

    if (w > 1) {
      level = (uint8_t)(((uint32_t)i * 63u) / (uint32_t)(w - 1u));
    }

    switch (channel) {
      case 'R':
        r6 = level;
        break;
      case 'G':
        g6 = level;
        break;
      case 'B':
        b6 = level;
        break;
    }

    ili9486l_fill_rect_rgb666((uint16_t)(x + i), y, 1, h, r6, g6, b6);
  }
}

static void show_color_test_screen(void) {
  const lcd_color_t background = LCD_RGB666(0x02, 0x04, 0x05);
  const lcd_color_t title_bg = LCD_RGB666(0x04, 0x06, 0x08);
  const lcd_color_t frame = LCD_RGB666(0x00, 0x01, 0x02);
  const lcd_color_t red_label = LCD_RGB666(0x3F, 0x1C, 0x1C);
  const lcd_color_t green_label = LCD_RGB666(0x1C, 0x3F, 0x24);
  const lcd_color_t blue_label = LCD_RGB666(0x1C, 0x28, 0x3F);

  ili9486l_fill_screen(background);
  ili9486l_fill_rect(10, 10, 460, 34, title_bg);
  ili9486l_draw_string(22, 19, "RGB CHANNEL TEST 0..63", LCD_COLOR_WHITE, title_bg, 2);

  ili9486l_fill_rect(10, 52, 460, 254, frame);

  ili9486l_draw_string(24, 70, "R", red_label, frame, 3);
  ili9486l_draw_string(64, 76, "0", LCD_COLOR_WHITE, frame, 2);
  ili9486l_fill_rect(92, 78, 340, 26, LCD_COLOR_BLACK);
  draw_channel_gradient_strip(96, 82, 332, 18, 'R');
  ili9486l_draw_string(438, 76, "63", LCD_COLOR_WHITE, frame, 2);

  ili9486l_draw_string(24, 142, "G", green_label, frame, 3);
  ili9486l_draw_string(64, 148, "0", LCD_COLOR_WHITE, frame, 2);
  ili9486l_fill_rect(92, 150, 340, 26, LCD_COLOR_BLACK);
  draw_channel_gradient_strip(96, 154, 332, 18, 'G');
  ili9486l_draw_string(438, 148, "63", LCD_COLOR_WHITE, frame, 2);

  ili9486l_draw_string(24, 214, "B", blue_label, frame, 3);
  ili9486l_draw_string(64, 220, "0", LCD_COLOR_WHITE, frame, 2);
  ili9486l_fill_rect(92, 222, 340, 26, LCD_COLOR_BLACK);
  draw_channel_gradient_strip(96, 226, 332, 18, 'B');
  ili9486l_draw_string(438, 220, "63", LCD_COLOR_WHITE, frame, 2);

  ili9486l_draw_string(24, 274, "SPECTRUM", LCD_COLOR_WHITE, frame, 1);
  ili9486l_fill_rect(92, 274, 340, 20, LCD_COLOR_BLACK);
  draw_rgb_gradient_strip(96, 278, 332, 12);
  sleep_ms(3000);
}

static void draw_demo_screen(void) {
  const lcd_color_t background = LCD_RGB666(0x02, 0x04, 0x06);
  const lcd_color_t panel = LCD_RGB666(0x04, 0x08, 0x0A);
  const lcd_color_t accent = LCD_RGB666(0x0C, 0x05, 0x00);
  const lcd_color_t strip_frame = LCD_RGB666(0x01, 0x02, 0x03);

  ili9486l_fill_screen(background);

  ili9486l_fill_rect(10, 10, 460, 68, accent);
  ili9486l_fill_rect(10, 92, 220, 218, panel);
  ili9486l_fill_rect(250, 92, 220, 218, panel);

  ili9486l_draw_string(22, 22, "RP2040 + ILI9486L", LCD_COLOR_YELLOW, accent, 2);
  ili9486l_draw_string(22, 50, "LANDSCAPE 480X320", LCD_COLOR_WHITE, accent, 2);

  ili9486l_draw_string(24, 106, "5X7 FONT DEMO", LCD_COLOR_CYAN, panel, 2);
  ili9486l_draw_string(24, 134, "4-WIRE SPI", LCD_COLOR_WHITE, panel, 2);
  ili9486l_draw_string(24, 176, "SCK  GP14", LCD_COLOR_WHITE, panel, 2);
  ili9486l_draw_string(24, 204, "MOSI GP15", LCD_COLOR_WHITE, panel, 2);
  ili9486l_draw_string(24, 232, "DC   GP10", LCD_COLOR_GREEN, panel, 2);
  ili9486l_draw_string(24, 260, "RST  GP11", LCD_COLOR_GREEN, panel, 2);

  ili9486l_draw_string(264, 106, "TX ONLY SPI", LCD_COLOR_WHITE, panel, 2);
  ili9486l_draw_string(264, 134, "NO MISO PIN", LCD_COLOR_WHITE, panel, 2);
  ili9486l_draw_string(264, 162, "BLK  TO 3V3", LCD_COLOR_GREEN, panel, 2);
  ili9486l_draw_string(264, 190, "320X480 TFT", LCD_COLOR_CYAN, panel, 2);
  ili9486l_draw_string(264, 228, "HELLO,", LCD_COLOR_YELLOW, panel, 3);
  ili9486l_draw_string(264, 258, "PICO!", LCD_COLOR_YELLOW, panel, 3);

  ili9486l_fill_rect(20, 286, 440, 20, strip_frame);
  draw_rgb_gradient_strip(24, 290, 432, 12);
}

int main(void) {
  stdio_init_all();
  sleep_ms(1000);

  ili9486l_init();
  show_boot_logo();
  show_color_test_screen();
  draw_demo_screen();

  while (true) {
    tight_loop_contents();
  }
}
