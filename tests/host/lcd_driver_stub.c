#include "lcd_driver.h"

unsigned lcd_stub_begin_write_calls = 0u;
unsigned lcd_stub_wire_write_calls = 0u;
unsigned lcd_stub_wire_rect_calls = 0u;

void lcd_stub_reset_counters(void)
{
    lcd_stub_begin_write_calls = 0u;
    lcd_stub_wire_write_calls = 0u;
    lcd_stub_wire_rect_calls = 0u;
}

static uint16_t stub_width(lcd_driver_t *drv)
{
    (void)drv;
    return 480u;
}

static uint16_t stub_height(lcd_driver_t *drv)
{
    (void)drv;
    return 320u;
}

static void stub_fill_rect(lcd_driver_t *drv, uint16_t x, uint16_t y, uint16_t w, uint16_t h, lcd_color_t color)
{
    (void)drv;
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)color;
}

static void stub_fill_screen(lcd_driver_t *drv, lcd_color_t color)
{
    (void)drv;
    (void)color;
}

static void stub_draw_rgb666_wire_rect(lcd_driver_t *drv, const uint8_t *bmp, uint16_t x, uint16_t y, uint16_t w,
                                       uint16_t h)
{
    (void)drv;
    (void)bmp;
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    ++lcd_stub_wire_rect_calls;
}

static bool stub_begin_write(lcd_driver_t *drv, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    (void)drv;
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    ++lcd_stub_begin_write_calls;
    return true;
}

static void stub_write_pixels(lcd_driver_t *drv, const uint8_t *pixels, size_t pixel_count)
{
    (void)drv;
    (void)pixels;
    (void)pixel_count;
    ++lcd_stub_wire_write_calls;
}

static void stub_flush(lcd_driver_t *drv)
{
    (void)drv;
}

static const lcd_driver_ops_t g_stub_ops = {
    .width = stub_width,
    .height = stub_height,
    .fill_rect = stub_fill_rect,
    .fill_screen = stub_fill_screen,
    .draw_rgb666_wire_rect = stub_draw_rgb666_wire_rect,
    .begin_write = stub_begin_write,
    .write_pixels = stub_write_pixels,
    .flush = stub_flush,
};

static lcd_driver_t g_stub_driver = {
    .ops = &g_stub_ops,
};

lcd_driver_t *lcd_stub_get_driver(void)
{
    return &g_stub_driver;
}

lcd_driver_t *lcd_init(void)
{
    return &g_stub_driver;
}

void lcd_destroy(lcd_driver_t *drv)
{
    (void)drv;
}
