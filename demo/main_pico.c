#include "demo_screens.h"
#include "drivers/ili9486l/ili9486l.h"
#include "lcd_jpeg.h"
#include "lcd_text.h"
#include "vt100_terminal.h"

#include "pico/stdlib.h"

#include <stdio.h>

#ifndef ILI9486L_LCD_DEMO_RUN_FPS_TEST
#define ILI9486L_LCD_DEMO_RUN_FPS_TEST 1
#endif

extern const uint8_t g_logo_jpg_start[];
extern const uint8_t g_logo_jpg_end[];

static uint64_t pico_time_us(void)
{
    return time_us_64();
}

static void terminal_stdio_output(const char *data, size_t len, void *user_data)
{
    (void)user_data;

    for (size_t i = 0; i < len; ++i) {
        putchar_raw((uint8_t)data[i]);
    }
}

static void show_boot_logo(lcd_driver_t *drv)
{
    const size_t logo_size = (size_t)(g_logo_jpg_end - g_logo_jpg_start);

    if (!lcd_jpeg_draw(drv, g_logo_jpg_start, logo_size, 0u, 0u)) {
        lcd_fill_screen(drv, LCD_COLOR_BLACK);
        return;
    }

    sleep_ms(3000);
}

static void draw_native_scroll_demo_screen(lcd_driver_t *drv, uint16_t top_fixed, uint16_t bottom_fixed)
{
    static const uint8_t band_colors[][3] = {
        {63u, 10u, 10u},
        {63u, 24u, 8u},
        {63u, 40u, 8u},
        {48u, 56u, 8u},
        {20u, 63u, 16u},
        {8u, 63u, 40u},
        {8u, 52u, 63u},
        {20u, 28u, 63u},
        {40u, 12u, 63u},
        {56u, 8u, 44u},
        {63u, 8u, 24u},
        {36u, 36u, 36u},
    };
    const uint16_t scroll_area = (uint16_t)(lcd_height(drv) - top_fixed - bottom_fixed);
    const uint16_t band_height = 24u;
    const lcd_color_t header_bg = LCD_RGB666(0x03, 0x05, 0x0B);
    const lcd_color_t footer_bg = LCD_RGB666(0x08, 0x03, 0x03);
    char label[24];

    lcd_fill_screen(drv, LCD_COLOR_BLACK);

    lcd_fill_rect(drv, 0, 0, lcd_width(drv), top_fixed, header_bg);
    lcd_draw_string(drv, 16, 6, "NATIVE SCROLL AXIS", LCD_COLOR_WHITE, header_bg, 2);

    for (uint16_t y = 0, band = 0; y < scroll_area; y = (uint16_t)(y + band_height), ++band) {
        const uint16_t actual_y = (uint16_t)(top_fixed + y);
        const uint16_t height = (uint16_t)((y + band_height <= scroll_area) ? band_height : (scroll_area - y));
        const uint8_t *color = band_colors[band % (sizeof(band_colors) / sizeof(band_colors[0]))];
        const lcd_color_t bg = LCD_RGB666(color[0], color[1], color[2]);

        lcd_fill_rect(drv, 0, actual_y, lcd_width(drv), height, bg);
        snprintf(label, sizeof(label), "ROW %03u", (unsigned)band);
        lcd_draw_string(drv, 16, (uint16_t)(actual_y + 6), label, LCD_COLOR_BLACK, bg, 2);
    }

    lcd_fill_rect(drv, 0, (uint16_t)(lcd_height(drv) - bottom_fixed), lcd_width(drv), bottom_fixed, footer_bg);
    lcd_draw_string(drv, 16, (uint16_t)(lcd_height(drv) - bottom_fixed + 6), "LANDSCAPE => HORIZONTAL",
                    LCD_COLOR_WHITE, footer_bg, 2);
}

static void show_native_scroll_demo(lcd_driver_t *drv)
{
    const uint16_t top_fixed = 28u;
    const uint16_t bottom_fixed = 28u;
    const uint16_t scroll_area = (uint16_t)(lcd_height(drv) - top_fixed - bottom_fixed);

    draw_native_scroll_demo_screen(drv, top_fixed, bottom_fixed);

    if (!ili9486l_configure_vertical_scroll(top_fixed, scroll_area, bottom_fixed)) {
        return;
    }

    sleep_ms(500);
    (void)ili9486l_set_vertical_scroll_start((uint16_t)(scroll_area / 4u));
    sleep_ms(500);
    (void)ili9486l_set_vertical_scroll_start((uint16_t)(scroll_area / 2u));
    sleep_ms(500);
    (void)ili9486l_set_vertical_scroll_start((uint16_t)((scroll_area * 3u) / 4u));
    sleep_ms(500);

    for (uint16_t offset = 0; offset < scroll_area; ++offset) {
        (void)ili9486l_set_vertical_scroll_start(offset);
        sleep_ms(6);
    }

    sleep_ms(200);
    (void)ili9486l_set_vertical_scroll_start(0);
    sleep_ms(500);
    ili9486l_reset_vertical_scroll();
}

int main(void)
{
    static vt100_terminal_t terminal;
    lcd_driver_t *drv = NULL;
    uint16_t terminal_origin_y = 0;
    uint32_t last_terminal_tick_ms = 0u;

    stdio_init_all();
    sleep_ms(1000);

    drv = lcd_init();
    demo_set_time_fn(pico_time_us);

    show_boot_logo(drv);
#if ILI9486L_LCD_DEMO_RUN_FPS_TEST
    demo_show_full_redraw_fps_test(drv);
    sleep_ms(1500);
    demo_show_terminal_benchmark_results(drv,
        (uint16_t)((lcd_height(drv) - VT100_TERMINAL_HEIGHT_PIXELS) / 2u), &terminal);
    sleep_ms(1800);
#endif
    terminal_origin_y = (uint16_t)((lcd_height(drv) - VT100_TERMINAL_HEIGHT_PIXELS) / 2u);
    lcd_fill_screen(drv, LCD_COLOR_BLACK);
    vt100_terminal_init(&terminal, drv, 0u, terminal_origin_y);
    vt100_terminal_set_output(&terminal, terminal_stdio_output, NULL);
    (void)vt100_terminal_getch(&terminal, PICO_ERROR_TIMEOUT);
    vt100_terminal_write(&terminal, "\x1b[2J\x1b[H");
    vt100_terminal_write(&terminal, "ILI9486L VT100 TERMINAL 80X34 + STATUS\r\n");
    vt100_terminal_write(&terminal, "Last line is reserved for status. Ctrl+E enters local command mode.\r\n");
    vt100_terminal_write(&terminal, "Ctrl+E 1/2/3 switches 80x34 / 80x30 / 80x24. Ctrl+E S/P switches SCROLL / PAGED.\r\n");
    vt100_terminal_write(&terminal, "In paged mode the status line asks for SPACE before a new page.\r\n");
    vt100_terminal_write(&terminal, "Commands are handled locally by vt100_terminal_getch().\r\n");
    vt100_terminal_write(&terminal, "Supported terminal core: ANSI, VT100, VT52, DEC graphics.\r\n");
    vt100_terminal_write(&terminal, "\r\n");
    vt100_terminal_write(&terminal, "\x1b[32mREADY\x1b[0m> ");
    last_terminal_tick_ms = (uint32_t)to_ms_since_boot(get_absolute_time());

    while (true) {
        const int ch = getchar_timeout_us(0);
        const uint32_t now_ms = (uint32_t)to_ms_since_boot(get_absolute_time());

        vt100_terminal_tick(&terminal, now_ms - last_terminal_tick_ms);
        last_terminal_tick_ms = now_ms;

        (void)vt100_terminal_getch(&terminal, ch);

        tight_loop_contents();
    }
}
