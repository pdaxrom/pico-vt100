#ifndef LCD_TEXT_H
#define LCD_TEXT_H

#include "lcd_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

void lcd_draw_char(lcd_driver_t *drv, uint16_t x, uint16_t y, char c, lcd_color_t fg, lcd_color_t bg, uint8_t scale);
void lcd_draw_string(lcd_driver_t *drv, uint16_t x, uint16_t y, const char *text, lcd_color_t fg, lcd_color_t bg,
                     uint8_t scale);

#ifdef __cplusplus
}
#endif

#endif
