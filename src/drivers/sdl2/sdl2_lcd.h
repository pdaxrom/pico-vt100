#ifndef SDL2_LCD_H
#define SDL2_LCD_H

#include "lcd_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

lcd_driver_t *sdl2_lcd_create(uint16_t width, uint16_t height, uint8_t scale);
void sdl2_lcd_destroy(lcd_driver_t *drv);
void sdl2_lcd_present(lcd_driver_t *drv);

#ifdef __cplusplus
}
#endif

#endif
