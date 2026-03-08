#ifndef ILI9486L_H
#define ILI9486L_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define LCD_NATIVE_WIDTH  320u
#define LCD_NATIVE_HEIGHT 480u

#define LCD_RGB565(r, g, b) \
  (uint16_t)((((uint16_t)(r) & 0xF8u) << 8) | (((uint16_t)(g) & 0xFCu) << 3) | (((uint16_t)(b) & 0xF8u) >> 3))

#define LCD_COLOR_BLACK   0x0000u
#define LCD_COLOR_WHITE   0xFFFFu
#define LCD_COLOR_RED     0xF800u
#define LCD_COLOR_GREEN   0x07E0u
#define LCD_COLOR_BLUE    0x001Fu
#define LCD_COLOR_CYAN    0x07FFu
#define LCD_COLOR_MAGENTA 0xF81Fu
#define LCD_COLOR_YELLOW  0xFFE0u

void ili9486l_init(void);
void ili9486l_set_rotation(uint8_t rotation);
uint16_t ili9486l_width(void);
uint16_t ili9486l_height(void);

void ili9486l_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
void ili9486l_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ili9486l_fill_rect_rgb666(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t r6, uint8_t g6, uint8_t b6);
void ili9486l_fill_screen(uint16_t color);
bool ili9486l_begin_write(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void ili9486l_write_rgb565_pixels(const uint16_t *pixels, size_t pixel_count);
void ili9486l_draw_rgb565_rect(const uint16_t *bitmap, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void ili9486l_draw_rgb565_bitmap(const uint8_t *bitmap, uint16_t w, uint16_t h);
void ili9486l_draw_char(uint16_t x, uint16_t y, char c, uint16_t fg, uint16_t bg, uint8_t scale);
void ili9486l_draw_string(uint16_t x, uint16_t y, const char *text, uint16_t fg, uint16_t bg, uint8_t scale);

#endif
