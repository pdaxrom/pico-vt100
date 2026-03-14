#ifndef ILI9486L_H
#define ILI9486L_H

#include "lcd_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LCD_NATIVE_WIDTH  320u
#define LCD_NATIVE_HEIGHT 480u

void ili9486l_init(void);
lcd_driver_t *ili9486l_get_driver(void);

void ili9486l_set_rotation(uint8_t rotation);
bool ili9486l_configure_vertical_scroll(uint16_t top_fixed, uint16_t scroll_area, uint16_t bottom_fixed);
bool ili9486l_set_vertical_scroll_start(uint16_t start);
bool ili9486l_scroll_vertical_by(int16_t delta);
void ili9486l_reset_vertical_scroll(void);

#ifdef __cplusplus
}
#endif

#endif
