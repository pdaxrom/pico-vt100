#ifndef LCD_JPEG_H
#define LCD_JPEG_H

#include "lcd_driver.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t width;
    uint16_t height;
} lcd_jpeg_info_t;

bool lcd_jpeg_get_info(const uint8_t *jpeg_data, size_t jpeg_size, lcd_jpeg_info_t *out_info);
bool lcd_jpeg_draw(lcd_driver_t *drv, const uint8_t *jpeg_data, size_t jpeg_size, uint16_t x, uint16_t y);

#ifdef __cplusplus
}
#endif

#endif
