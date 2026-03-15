#include "lcd_text.h"

#include "font5x7.h"

#define LCD_TEXT_FAST_SCALE_MAX 4u

static void lcd_draw_char_slow(lcd_driver_t *drv, uint16_t x, uint16_t y, char c, lcd_color_t fg, lcd_color_t bg,
                               uint8_t scale)
{
    const uint8_t *glyph_rows = font5x7_get_cell6x9_row_masks(c);

    lcd_fill_rect(drv, x, y, (uint16_t)(6u * scale), (uint16_t)(FONT5X7_CELL6X9_HEIGHT * scale), bg);

    for (uint8_t row = 0; row < FONT5X7_CELL6X9_HEIGHT; ++row) {
        const uint8_t row_mask = glyph_rows[row];

        for (uint8_t col = 0; col < 6u; ++col) {
            if ((row_mask & (1u << col)) != 0u) {
                lcd_fill_rect(drv, (uint16_t)(x + col * scale), (uint16_t)(y + row * scale), scale, scale, fg);
            }
        }
    }
}

void lcd_draw_char(lcd_driver_t *drv, uint16_t x, uint16_t y, char c, lcd_color_t fg, lcd_color_t bg, uint8_t scale)
{
    const uint8_t *glyph_rows;
    const uint16_t glyph_w = (uint16_t)(6u * scale);
    const uint16_t glyph_h = (uint16_t)(FONT5X7_CELL6X9_HEIGHT * scale);
    uint8_t row_pixels[6u * LCD_TEXT_FAST_SCALE_MAX * 3u];
    uint8_t fg_wire[3];
    uint8_t bg_wire[3];

    if (scale == 0 || x >= lcd_width(drv) || y >= lcd_height(drv)) {
        return;
    }

    glyph_rows = font5x7_get_cell6x9_row_masks(c);
    if (scale > LCD_TEXT_FAST_SCALE_MAX || (uint32_t)x + glyph_w > lcd_width(drv) ||
        (uint32_t)y + glyph_h > lcd_height(drv)) {
        lcd_draw_char_slow(drv, x, y, c, fg, bg, scale);
        return;
    }

    lcd_color_to_rgb666_wire(fg, fg_wire);
    lcd_color_to_rgb666_wire(bg, bg_wire);

    if (!lcd_begin_write(drv, x, y, glyph_w, glyph_h)) {
        return;
    }

    for (uint8_t glyph_row = 0; glyph_row < FONT5X7_CELL6X9_HEIGHT; ++glyph_row) {
        const uint8_t row_mask = glyph_rows[glyph_row];

        for (uint8_t sy = 0; sy < scale; ++sy) {
            uint8_t *dst = row_pixels;

            for (uint8_t col = 0; col < 6u; ++col) {
                const bool pixel_on = (row_mask & (1u << col)) != 0u;
                const uint8_t *color = pixel_on ? fg_wire : bg_wire;

                for (uint8_t sx = 0; sx < scale; ++sx) {
                    dst[0] = color[0];
                    dst[1] = color[1];
                    dst[2] = color[2];
                    dst += 3;
                }
            }

            lcd_write_pixels(drv, row_pixels, glyph_w);
        }
    }

    lcd_flush(drv);
}

void lcd_draw_string(lcd_driver_t *drv, uint16_t x, uint16_t y, const char *text, lcd_color_t fg, lcd_color_t bg,
                     uint8_t scale)
{
    const uint16_t start_x = x;
    const uint16_t advance_x = (uint16_t)(6u * scale);
    const uint16_t advance_y = (uint16_t)(FONT5X7_CELL6X9_HEIGHT * scale);

    if (text == NULL || scale == 0) {
        return;
    }

    while (*text != '\0') {
        if (*text == '\n') {
            x = start_x;
            y = (uint16_t)(y + advance_y);
            ++text;
            continue;
        }

        if ((uint32_t)x + advance_x > lcd_width(drv)) {
            x = start_x;
            y = (uint16_t)(y + advance_y);
        }

        if ((uint32_t)y + advance_y > lcd_height(drv)) {
            break;
        }

        lcd_draw_char(drv, x, y, *text, fg, bg, scale);
        x = (uint16_t)(x + advance_x);
        ++text;
    }
}
