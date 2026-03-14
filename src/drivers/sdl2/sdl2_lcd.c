#include "sdl2_lcd.h"

#include <SDL.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    lcd_driver_t base;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    uint8_t *pixels;
    uint16_t width;
    uint16_t height;
    uint8_t scale;
    uint16_t write_x;
    uint16_t write_y;
    uint16_t write_w;
    uint16_t write_h;
    uint32_t write_offset;
    bool texture_dirty;
} sdl2_lcd_t;

static sdl2_lcd_t *to_sdl2(lcd_driver_t *drv)
{
    return (sdl2_lcd_t *)drv;
}

static uint16_t sdl2_width(lcd_driver_t *drv)
{
    return to_sdl2(drv)->width;
}

static uint16_t sdl2_height(lcd_driver_t *drv)
{
    return to_sdl2(drv)->height;
}

static void sdl2_set_pixel(sdl2_lcd_t *lcd, uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b)
{
    if (x < lcd->width && y < lcd->height) {
        const size_t offset = ((size_t)y * lcd->width + x) * 3u;

        lcd->pixels[offset + 0] = r;
        lcd->pixels[offset + 1] = g;
        lcd->pixels[offset + 2] = b;
    }
}

static void sdl2_fill_rect(lcd_driver_t *drv, uint16_t x, uint16_t y, uint16_t w, uint16_t h, lcd_color_t color)
{
    sdl2_lcd_t *lcd = to_sdl2(drv);
    uint8_t wire[3];

    lcd_color_to_rgb666_wire(color, wire);

    for (uint16_t py = y; py < y + h && py < lcd->height; ++py) {
        for (uint16_t px = x; px < x + w && px < lcd->width; ++px) {
            sdl2_set_pixel(lcd, px, py, wire[0], wire[1], wire[2]);
        }
    }

    lcd->texture_dirty = true;
}

static void sdl2_fill_screen(lcd_driver_t *drv, lcd_color_t color)
{
    sdl2_lcd_t *lcd = to_sdl2(drv);

    sdl2_fill_rect(drv, 0, 0, lcd->width, lcd->height, color);
}

static void sdl2_draw_rgb666_wire_rect(lcd_driver_t *drv, const uint8_t *bmp, uint16_t x, uint16_t y, uint16_t w,
                                       uint16_t h)
{
    sdl2_lcd_t *lcd = to_sdl2(drv);
    const uint8_t *src = bmp;

    for (uint16_t py = 0; py < h; ++py) {
        for (uint16_t px = 0; px < w; ++px) {
            sdl2_set_pixel(lcd, (uint16_t)(x + px), (uint16_t)(y + py), src[0], src[1], src[2]);
            src += 3;
        }
    }

    lcd->texture_dirty = true;
}

static bool sdl2_begin_write(lcd_driver_t *drv, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    sdl2_lcd_t *lcd = to_sdl2(drv);

    if (w == 0 || h == 0 || x >= lcd->width || y >= lcd->height) {
        return false;
    }

    if ((uint32_t)x + w > lcd->width || (uint32_t)y + h > lcd->height) {
        return false;
    }

    lcd->write_x = x;
    lcd->write_y = y;
    lcd->write_w = w;
    lcd->write_h = h;
    lcd->write_offset = 0;
    return true;
}

static void sdl2_write_pixels(lcd_driver_t *drv, const uint8_t *pixels, size_t pixel_count)
{
    sdl2_lcd_t *lcd = to_sdl2(drv);
    const uint8_t *src = pixels;
    const uint32_t total = (uint32_t)lcd->write_w * lcd->write_h;

    for (size_t i = 0; i < pixel_count && lcd->write_offset < total; ++i) {
        const uint16_t local_x = (uint16_t)(lcd->write_offset % lcd->write_w);
        const uint16_t local_y = (uint16_t)(lcd->write_offset / lcd->write_w);

        sdl2_set_pixel(lcd, (uint16_t)(lcd->write_x + local_x), (uint16_t)(lcd->write_y + local_y),
                       src[0], src[1], src[2]);
        src += 3;
        ++lcd->write_offset;
    }

    lcd->texture_dirty = true;
}

static void sdl2_flush(lcd_driver_t *drv)
{
    (void)drv;
}

static const lcd_driver_ops_t g_sdl2_ops = {
    .width = sdl2_width,
    .height = sdl2_height,
    .fill_rect = sdl2_fill_rect,
    .fill_screen = sdl2_fill_screen,
    .draw_rgb666_wire_rect = sdl2_draw_rgb666_wire_rect,
    .begin_write = sdl2_begin_write,
    .write_pixels = sdl2_write_pixels,
    .flush = sdl2_flush,
};

lcd_driver_t *sdl2_lcd_create(uint16_t width, uint16_t height, uint8_t scale)
{
    sdl2_lcd_t *lcd = NULL;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        return NULL;
    }

    lcd = (sdl2_lcd_t *)calloc(1, sizeof(*lcd));
    if (lcd == NULL) {
        return NULL;
    }

    lcd->base.ops = &g_sdl2_ops;
    lcd->width = width;
    lcd->height = height;
    lcd->scale = scale;

    lcd->window = SDL_CreateWindow(
        "LCD Demo (SDL2)",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        (int)width * scale,
        (int)height * scale,
        SDL_WINDOW_SHOWN);

    if (lcd->window == NULL) {
        free(lcd);
        return NULL;
    }

    lcd->renderer = SDL_CreateRenderer(lcd->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (lcd->renderer == NULL) {
        SDL_DestroyWindow(lcd->window);
        free(lcd);
        return NULL;
    }

    lcd->texture = SDL_CreateTexture(lcd->renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING,
                                     (int)width, (int)height);
    if (lcd->texture == NULL) {
        SDL_DestroyRenderer(lcd->renderer);
        SDL_DestroyWindow(lcd->window);
        free(lcd);
        return NULL;
    }

    lcd->pixels = (uint8_t *)calloc((size_t)width * height * 3u, 1);
    if (lcd->pixels == NULL) {
        SDL_DestroyTexture(lcd->texture);
        SDL_DestroyRenderer(lcd->renderer);
        SDL_DestroyWindow(lcd->window);
        free(lcd);
        return NULL;
    }

    lcd->texture_dirty = true;
    return &lcd->base;
}

void sdl2_lcd_destroy(lcd_driver_t *drv)
{
    sdl2_lcd_t *lcd = NULL;

    if (drv == NULL) {
        return;
    }

    lcd = to_sdl2(drv);
    free(lcd->pixels);
    SDL_DestroyTexture(lcd->texture);
    SDL_DestroyRenderer(lcd->renderer);
    SDL_DestroyWindow(lcd->window);
    free(lcd);
    SDL_Quit();
}

#ifndef SDL2_LCD_SCALE
#define SDL2_LCD_SCALE 2
#endif

lcd_driver_t *lcd_init(void)
{
    return sdl2_lcd_create(LCD_DISPLAY_WIDTH, LCD_DISPLAY_HEIGHT, SDL2_LCD_SCALE);
}

void lcd_destroy(lcd_driver_t *drv)
{
    sdl2_lcd_destroy(drv);
}

void sdl2_lcd_present(lcd_driver_t *drv)
{
    sdl2_lcd_t *lcd = NULL;

    if (drv == NULL) {
        return;
    }

    lcd = to_sdl2(drv);
    if (!lcd->texture_dirty) {
        return;
    }

    SDL_UpdateTexture(lcd->texture, NULL, lcd->pixels, (int)lcd->width * 3);
    SDL_RenderClear(lcd->renderer);
    SDL_RenderCopy(lcd->renderer, lcd->texture, NULL, NULL);
    SDL_RenderPresent(lcd->renderer);
    lcd->texture_dirty = false;
}
