#include "demo_screens.h"

#include "lcd_text.h"

#include <stdio.h>
#include <string.h>

static uint64_t default_time_us(void)
{
    return 0u;
}

static demo_time_us_fn g_time_fn = default_time_us;

void demo_set_time_fn(demo_time_us_fn fn)
{
    g_time_fn = fn ? fn : default_time_us;
}

static void format_fixed_x10(char *dst, size_t dst_size, uint32_t value_x10)
{
    snprintf(dst, dst_size, "%lu.%01lu", (unsigned long)(value_x10 / 10u), (unsigned long)(value_x10 % 10u));
}

/* --- Full-redraw benchmark --- */

static uint8_t g_full_redraw_scanline[480u * 3u];

static void draw_full_redraw_benchmark_frame(lcd_driver_t *drv, uint32_t frame_index)
{
    const uint16_t width = lcd_width(drv);
    const uint16_t height = lcd_height(drv);

    if (!lcd_begin_write(drv, 0u, 0u, width, height)) {
        return;
    }

    for (uint16_t y = 0; y < height; ++y) {
        const uint8_t y6 = (height > 1u) ? (uint8_t)(((uint32_t)y * 63u) / (uint32_t)(height - 1u)) : 0u;

        for (uint16_t x = 0; x < width; ++x) {
            const uint8_t x6 = (width > 1u) ? (uint8_t)(((uint32_t)x * 63u) / (uint32_t)(width - 1u)) : 0u;
            const uint8_t r6 = (uint8_t)((x6 + frame_index * 3u) & 0x3Fu);
            const uint8_t g6 = (uint8_t)((y6 + frame_index * 5u) & 0x3Fu);
            const uint8_t b6 = (uint8_t)(((x6 ^ y6) + frame_index * 7u) & 0x3Fu);
            const size_t offset = (size_t)x * 3u;

            g_full_redraw_scanline[offset + 0u] = (uint8_t)(r6 << 2);
            g_full_redraw_scanline[offset + 1u] = (uint8_t)(g6 << 2);
            g_full_redraw_scanline[offset + 2u] = (uint8_t)(b6 << 2);
        }

        lcd_write_pixels(drv, g_full_redraw_scanline, width);
    }

    lcd_flush(drv);
}

void demo_run_full_redraw_fps_test(lcd_driver_t *drv, benchmark_result_t *result)
{
    static const uint32_t k_frame_count = 12u;
    const uint64_t start_us = g_time_fn();
    uint64_t elapsed_us = 0u;

    if (result == NULL) {
        return;
    }

    for (uint32_t frame = 0; frame < k_frame_count; ++frame) {
        draw_full_redraw_benchmark_frame(drv, frame);
    }

    elapsed_us = g_time_fn() - start_us;
    if (elapsed_us == 0u) {
        elapsed_us = 1u;
    }

    result->rate_x10 = (uint32_t)(((uint64_t)k_frame_count * 10000000u) / elapsed_us);
    result->ms_x10 = (uint32_t)((elapsed_us * 10u) / ((uint64_t)k_frame_count * 1000u));
}

void demo_show_full_redraw_fps_test(lcd_driver_t *drv)
{
    const uint16_t width = lcd_width(drv);
    const uint16_t height = lcd_height(drv);
    benchmark_result_t result;
    char fps_text[16];
    char ms_text[16];
    char line[48];
    const lcd_color_t panel = LCD_RGB666(0x06, 0x08, 0x0C);

    demo_run_full_redraw_fps_test(drv, &result);
    format_fixed_x10(fps_text, sizeof(fps_text), result.rate_x10);
    format_fixed_x10(ms_text, sizeof(ms_text), result.ms_x10);

    printf("FULL REDRAW FPS TEST: %s FPS, %s ms/frame, %u frames at %ux%u\n",
           fps_text, ms_text, 12u, width, height);

    lcd_fill_screen(drv, LCD_RGB666(0x02, 0x03, 0x06));
    lcd_fill_rect(drv, 16, 18, 448, 284, panel);
    lcd_draw_string(drv, 34, 34, "FULL REDRAW TEST", LCD_COLOR_YELLOW, panel, 2);
    lcd_draw_string(drv, 34, 74, "STREAMING RGB666", LCD_COLOR_WHITE, panel, 2);

    snprintf(line, sizeof(line), "%s FPS", fps_text);
    lcd_draw_string(drv, 34, 128, line, LCD_COLOR_CYAN, panel, 3);

    snprintf(line, sizeof(line), "%s MS/FRAME", ms_text);
    lcd_draw_string(drv, 34, 180, line, LCD_COLOR_WHITE, panel, 2);

    snprintf(line, sizeof(line), "%u FRAMES @ %ux%u", 12u, width, height);
    lcd_draw_string(drv, 34, 224, line, LCD_COLOR_GREEN, panel, 2);
    lcd_draw_string(drv, 34, 268, "RESULT ALSO IN STDIO", LCD_COLOR_WHITE, panel, 1);
}

/* --- VT100 benchmarks --- */

static void prepare_terminal_render_benchmark(lcd_driver_t *drv, vt100_terminal_t *terminal, uint16_t origin_y)
{
    static const char k_pattern[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789[]{}<>+-*/";

    vt100_terminal_init(terminal, drv, 0u, origin_y);

    for (uint8_t row = 0; row < VT100_TERMINAL_ROWS; ++row) {
        for (uint8_t col = 0; col < VT100_TERMINAL_COLS; ++col) {
            vt100_terminal_cell_t *cell = &terminal->cells[row][col];
            const uint8_t fg = (uint8_t)((8u + row + col) & 0x0Fu);
            const uint8_t bg = (uint8_t)(((row / 5u) + (col / 20u)) & 0x07u);

            cell->ch = k_pattern[(row * 7u + col) % (sizeof(k_pattern) - 1u)];
            cell->attr = (uint8_t)(((bg & 0x0Fu) << 4) | (fg & 0x0Fu));
            cell->style = 0u;
            if (((row + col) % 9u) == 0u) {
                cell->style |= 0x01u;
            }
            if (((row + col) % 11u) == 0u) {
                cell->style |= 0x04u;
            }
            cell->charset = 0u;
        }
    }

    terminal->cursor_row = (uint8_t)(VT100_TERMINAL_ROWS / 2u);
    terminal->cursor_col = (uint8_t)(VT100_TERMINAL_COLS / 2u);
    terminal->wrap_pending = false;
    terminal->blink_visible = true;
}

static size_t build_terminal_benchmark_line(char *dst, uint32_t line_index)
{
    static const char k_pattern[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789[]{}<>|+-*/";
    char prefix[16];
    int prefix_len = snprintf(prefix, sizeof(prefix), "%04lu ", (unsigned long)(line_index % 10000u));

    if (prefix_len < 0) {
        prefix_len = 0;
    }

    for (uint16_t i = 0; i < VT100_TERMINAL_COLS; ++i) {
        if (i < (uint16_t)prefix_len) {
            dst[i] = prefix[i];
        } else {
            dst[i] = k_pattern[(i + line_index) % (sizeof(k_pattern) - 1u)];
        }
    }

    dst[VT100_TERMINAL_COLS + 0u] = '\r';
    dst[VT100_TERMINAL_COLS + 1u] = '\n';
    return VT100_TERMINAL_COLS + 2u;
}

static void run_terminal_render_fps_test(lcd_driver_t *drv, uint16_t origin_y, vt100_terminal_t *terminal,
                                         benchmark_result_t *result)
{
    static const uint32_t k_frame_count = 6u;
    uint64_t elapsed_us = 0u;
    const uint64_t start_us = g_time_fn();

    if (result == NULL) {
        return;
    }

    prepare_terminal_render_benchmark(drv, terminal, origin_y);

    for (uint32_t frame = 0; frame < k_frame_count; ++frame) {
        vt100_terminal_render(terminal);
    }

    elapsed_us = g_time_fn() - start_us;
    if (elapsed_us == 0u) {
        elapsed_us = 1u;
    }

    result->rate_x10 = (uint32_t)(((uint64_t)k_frame_count * 10000000u) / elapsed_us);
    result->ms_x10 = (uint32_t)((elapsed_us * 10u) / ((uint64_t)k_frame_count * 1000u));
}

static void run_terminal_scroll_fps_test(lcd_driver_t *drv, uint16_t origin_y, vt100_terminal_t *terminal,
                                         benchmark_result_t *result)
{
    static const uint32_t k_scroll_count = 12u;
    char line[VT100_TERMINAL_COLS + 2u];
    uint64_t elapsed_us = 0u;

    if (result == NULL) {
        return;
    }

    vt100_terminal_init(terminal, drv, 0u, origin_y);

    for (uint32_t row = 0; row < (uint32_t)(VT100_TERMINAL_ROWS - 1u); ++row) {
        const size_t len = build_terminal_benchmark_line(line, row);
        vt100_terminal_write_n(terminal, line, len);
    }

    {
        const uint64_t start_us = g_time_fn();

        for (uint32_t row = 0; row < k_scroll_count; ++row) {
            const size_t len = build_terminal_benchmark_line(line, 1000u + row);
            vt100_terminal_write_n(terminal, line, len);
        }

        elapsed_us = g_time_fn() - start_us;
    }

    if (elapsed_us == 0u) {
        elapsed_us = 1u;
    }

    result->rate_x10 = (uint32_t)(((uint64_t)k_scroll_count * 10000000u) / elapsed_us);
    result->ms_x10 = (uint32_t)((elapsed_us * 10u) / ((uint64_t)k_scroll_count * 1000u));
}

void demo_show_terminal_benchmark_results(lcd_driver_t *drv, uint16_t origin_y, vt100_terminal_t *scratch)
{
    benchmark_result_t render_result;
    benchmark_result_t scroll_result;
    char render_rate_text[16];
    char render_ms_text[16];
    char scroll_rate_text[16];
    char scroll_ms_text[16];
    char line[48];
    const lcd_color_t background = LCD_RGB666(0x03, 0x03, 0x06);
    const lcd_color_t panel = LCD_RGB666(0x06, 0x08, 0x0C);

    run_terminal_render_fps_test(drv, origin_y, scratch, &render_result);
    run_terminal_scroll_fps_test(drv, origin_y, scratch, &scroll_result);

    format_fixed_x10(render_rate_text, sizeof(render_rate_text), render_result.rate_x10);
    format_fixed_x10(render_ms_text, sizeof(render_ms_text), render_result.ms_x10);
    format_fixed_x10(scroll_rate_text, sizeof(scroll_rate_text), scroll_result.rate_x10);
    format_fixed_x10(scroll_ms_text, sizeof(scroll_ms_text), scroll_result.ms_x10);

    printf("VT100 FULL RENDER TEST: %s FPS, %s ms/frame at 80x35\n",
           render_rate_text, render_ms_text);
    printf("VT100 SCROLL TEST: %s lines/s, %s ms/line at 80 cols\n",
           scroll_rate_text, scroll_ms_text);

    lcd_fill_screen(drv, background);
    lcd_fill_rect(drv, 16, 18, 448, 284, panel);
    lcd_draw_string(drv, 34, 34, "VT100 BENCHMARK", LCD_COLOR_YELLOW, panel, 2);

    snprintf(line, sizeof(line), "RENDER %s FPS", render_rate_text);
    lcd_draw_string(drv, 34, 88, line, LCD_COLOR_CYAN, panel, 2);
    snprintf(line, sizeof(line), "%s MS/FRAME", render_ms_text);
    lcd_draw_string(drv, 34, 116, line, LCD_COLOR_WHITE, panel, 2);

    snprintf(line, sizeof(line), "SCROLL %s L/S", scroll_rate_text);
    lcd_draw_string(drv, 34, 172, line, LCD_COLOR_GREEN, panel, 2);
    snprintf(line, sizeof(line), "%s MS/LINE", scroll_ms_text);
    lcd_draw_string(drv, 34, 200, line, LCD_COLOR_WHITE, panel, 2);

    lcd_draw_string(drv, 34, 254, "FULL RESULT ALSO IN STDIO", LCD_COLOR_WHITE, panel, 1);
}

/* --- Gradient helpers --- */

static void draw_rgb_gradient_strip(lcd_driver_t *drv, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
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

        lcd_fill_rect(drv, (uint16_t)(x + i), y, 1, h, LCD_RGB666(red, green, blue));
    }
}

static void draw_channel_gradient_strip(lcd_driver_t *drv, uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                                        char channel)
{
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
        case 'R': r6 = level; break;
        case 'G': g6 = level; break;
        case 'B': b6 = level; break;
        }

        lcd_fill_rect(drv, (uint16_t)(x + i), y, 1, h, LCD_RGB666(r6, g6, b6));
    }
}

void demo_show_color_test_screen(lcd_driver_t *drv)
{
    const lcd_color_t background = LCD_RGB666(0x02, 0x04, 0x05);
    const lcd_color_t title_bg = LCD_RGB666(0x04, 0x06, 0x08);
    const lcd_color_t frame = LCD_RGB666(0x00, 0x01, 0x02);
    const lcd_color_t red_label = LCD_RGB666(0x3F, 0x1C, 0x1C);
    const lcd_color_t green_label = LCD_RGB666(0x1C, 0x3F, 0x24);
    const lcd_color_t blue_label = LCD_RGB666(0x1C, 0x28, 0x3F);

    lcd_fill_screen(drv, background);
    lcd_fill_rect(drv, 10, 10, 460, 34, title_bg);
    lcd_draw_string(drv, 22, 19, "RGB CHANNEL TEST 0..63", LCD_COLOR_WHITE, title_bg, 2);

    lcd_fill_rect(drv, 10, 52, 460, 254, frame);

    lcd_draw_string(drv, 24, 70, "R", red_label, frame, 3);
    lcd_draw_string(drv, 64, 76, "0", LCD_COLOR_WHITE, frame, 2);
    lcd_fill_rect(drv, 92, 78, 340, 26, LCD_COLOR_BLACK);
    draw_channel_gradient_strip(drv, 96, 82, 332, 18, 'R');
    lcd_draw_string(drv, 438, 76, "63", LCD_COLOR_WHITE, frame, 2);

    lcd_draw_string(drv, 24, 142, "G", green_label, frame, 3);
    lcd_draw_string(drv, 64, 148, "0", LCD_COLOR_WHITE, frame, 2);
    lcd_fill_rect(drv, 92, 150, 340, 26, LCD_COLOR_BLACK);
    draw_channel_gradient_strip(drv, 96, 154, 332, 18, 'G');
    lcd_draw_string(drv, 438, 148, "63", LCD_COLOR_WHITE, frame, 2);

    lcd_draw_string(drv, 24, 214, "B", blue_label, frame, 3);
    lcd_draw_string(drv, 64, 220, "0", LCD_COLOR_WHITE, frame, 2);
    lcd_fill_rect(drv, 92, 222, 340, 26, LCD_COLOR_BLACK);
    draw_channel_gradient_strip(drv, 96, 226, 332, 18, 'B');
    lcd_draw_string(drv, 438, 220, "63", LCD_COLOR_WHITE, frame, 2);

    lcd_draw_string(drv, 24, 274, "SPECTRUM", LCD_COLOR_WHITE, frame, 1);
    lcd_fill_rect(drv, 92, 274, 340, 20, LCD_COLOR_BLACK);
    draw_rgb_gradient_strip(drv, 96, 278, 332, 12);
}

void demo_draw_demo_screen(lcd_driver_t *drv)
{
    const lcd_color_t background = LCD_RGB666(0x02, 0x04, 0x06);
    const lcd_color_t panel = LCD_RGB666(0x04, 0x08, 0x0A);
    const lcd_color_t accent = LCD_RGB666(0x0C, 0x05, 0x00);
    const lcd_color_t strip_frame = LCD_RGB666(0x01, 0x02, 0x03);

    lcd_fill_screen(drv, background);

    lcd_fill_rect(drv, 10, 10, 460, 68, accent);
    lcd_fill_rect(drv, 10, 92, 220, 218, panel);
    lcd_fill_rect(drv, 250, 92, 220, 218, panel);

    lcd_draw_string(drv, 22, 22, "RP2040 + ILI9486L", LCD_COLOR_YELLOW, accent, 2);
    lcd_draw_string(drv, 22, 50, "LANDSCAPE 480X320", LCD_COLOR_WHITE, accent, 2);

    lcd_draw_string(drv, 24, 106, "5X7 FONT DEMO", LCD_COLOR_CYAN, panel, 2);
    lcd_draw_string(drv, 24, 134, "4-WIRE SPI", LCD_COLOR_WHITE, panel, 2);
    lcd_draw_string(drv, 24, 176, "SCK  GP14", LCD_COLOR_WHITE, panel, 2);
    lcd_draw_string(drv, 24, 204, "MOSI GP15", LCD_COLOR_WHITE, panel, 2);
    lcd_draw_string(drv, 24, 232, "DC   GP10", LCD_COLOR_GREEN, panel, 2);
    lcd_draw_string(drv, 24, 260, "RST  GP11", LCD_COLOR_GREEN, panel, 2);

    lcd_draw_string(drv, 264, 106, "TX ONLY SPI", LCD_COLOR_WHITE, panel, 2);
    lcd_draw_string(drv, 264, 134, "NO MISO PIN", LCD_COLOR_WHITE, panel, 2);
    lcd_draw_string(drv, 264, 162, "BLK  TO 3V3", LCD_COLOR_GREEN, panel, 2);
    lcd_draw_string(drv, 264, 190, "320X480 TFT", LCD_COLOR_CYAN, panel, 2);
    lcd_draw_string(drv, 264, 228, "HELLO,", LCD_COLOR_YELLOW, panel, 3);
    lcd_draw_string(drv, 264, 258, "PICO!", LCD_COLOR_YELLOW, panel, 3);

    lcd_fill_rect(drv, 20, 286, 440, 20, strip_frame);
    draw_rgb_gradient_strip(drv, 24, 290, 432, 12);
}
