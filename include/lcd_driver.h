#ifndef LCD_DRIVER_H
#define LCD_DRIVER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef LCD_DISPLAY_WIDTH
#define LCD_DISPLAY_WIDTH  480u
#endif

#ifndef LCD_DISPLAY_HEIGHT
#define LCD_DISPLAY_HEIGHT 320u
#endif

typedef uint32_t lcd_color_t;

#define LCD_RGB666(r, g, b) \
  (lcd_color_t)((((uint32_t)(r) & 0x3Fu) << 12) | (((uint32_t)(g) & 0x3Fu) << 6) | ((uint32_t)(b) & 0x3Fu))

#define LCD_COLOR_BLACK   LCD_RGB666(0x00, 0x00, 0x00)
#define LCD_COLOR_WHITE   LCD_RGB666(0x3F, 0x3F, 0x3F)
#define LCD_COLOR_RED     LCD_RGB666(0x3F, 0x00, 0x00)
#define LCD_COLOR_GREEN   LCD_RGB666(0x00, 0x3F, 0x00)
#define LCD_COLOR_BLUE    LCD_RGB666(0x00, 0x00, 0x3F)
#define LCD_COLOR_CYAN    LCD_RGB666(0x00, 0x3F, 0x3F)
#define LCD_COLOR_MAGENTA LCD_RGB666(0x3F, 0x00, 0x3F)
#define LCD_COLOR_YELLOW  LCD_RGB666(0x3F, 0x3F, 0x00)

typedef struct lcd_driver lcd_driver_t;

typedef struct lcd_driver_ops {
    uint16_t (*width)(lcd_driver_t *drv);
    uint16_t (*height)(lcd_driver_t *drv);
    void (*fill_rect)(lcd_driver_t *drv, uint16_t x, uint16_t y, uint16_t w, uint16_t h, lcd_color_t color);
    void (*fill_screen)(lcd_driver_t *drv, lcd_color_t color);
    void (*draw_rgb666_wire_rect)(lcd_driver_t *drv, const uint8_t *bmp, uint16_t x, uint16_t y, uint16_t w,
                                  uint16_t h);
    bool (*begin_write)(lcd_driver_t *drv, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
    void (*write_pixels)(lcd_driver_t *drv, const uint8_t *pixels, size_t pixel_count);
    void (*flush)(lcd_driver_t *drv);
} lcd_driver_ops_t;

struct lcd_driver {
    const lcd_driver_ops_t *ops;
};

lcd_driver_t *lcd_init(void);
void lcd_destroy(lcd_driver_t *drv);

static inline void lcd_get_size(lcd_driver_t *drv, uint16_t *width, uint16_t *height)
{
    if (width != NULL) {
        *width = drv->ops->width(drv);
    }
    if (height != NULL) {
        *height = drv->ops->height(drv);
    }
}

static inline uint16_t lcd_width(lcd_driver_t *drv)
{
    return drv->ops->width(drv);
}

static inline uint16_t lcd_height(lcd_driver_t *drv)
{
    return drv->ops->height(drv);
}

static inline void lcd_fill_rect(lcd_driver_t *drv, uint16_t x, uint16_t y, uint16_t w, uint16_t h, lcd_color_t color)
{
    drv->ops->fill_rect(drv, x, y, w, h, color);
}

static inline void lcd_fill_screen(lcd_driver_t *drv, lcd_color_t color)
{
    drv->ops->fill_screen(drv, color);
}

static inline void lcd_draw_rgb666_wire_rect(lcd_driver_t *drv, const uint8_t *bmp, uint16_t x, uint16_t y,
                                             uint16_t w, uint16_t h)
{
    drv->ops->draw_rgb666_wire_rect(drv, bmp, x, y, w, h);
}

static inline bool lcd_begin_write(lcd_driver_t *drv, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    return drv->ops->begin_write(drv, x, y, w, h);
}

static inline void lcd_write_pixels(lcd_driver_t *drv, const uint8_t *pixels, size_t pixel_count)
{
    drv->ops->write_pixels(drv, pixels, pixel_count);
}

static inline void lcd_flush(lcd_driver_t *drv)
{
    drv->ops->flush(drv);
}

static inline void lcd_color_to_rgb666_wire(lcd_color_t color, uint8_t out[3])
{
    out[0] = (uint8_t)(((color >> 12) & 0x3Fu) << 2);
    out[1] = (uint8_t)(((color >> 6) & 0x3Fu) << 2);
    out[2] = (uint8_t)((color & 0x3Fu) << 2);
}

#ifdef __cplusplus
}
#endif

#endif
