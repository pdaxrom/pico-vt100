#include "lcd_jpeg.h"

#include "tjpgd/tjpgd.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define LCD_JPEG_WORKBUF_SIZE 4096u

typedef struct {
    const uint8_t *cursor;
    const uint8_t *end;
    lcd_driver_t *drv;
    uint16_t origin_x;
    uint16_t origin_y;
} lcd_jpeg_stream_t;

typedef union {
    uint32_t align;
    uint8_t bytes[LCD_JPEG_WORKBUF_SIZE];
} lcd_jpeg_workspace_t;

static lcd_jpeg_workspace_t g_lcd_jpeg_workspace;

static size_t lcd_jpeg_input(JDEC *decoder, uint8_t *buffer, size_t bytes_requested)
{
    lcd_jpeg_stream_t *stream = (lcd_jpeg_stream_t *)decoder->device;
    size_t available = (size_t)(stream->end - stream->cursor);

    if (bytes_requested > available) {
        bytes_requested = available;
    }

    if (buffer != NULL) {
        memcpy(buffer, stream->cursor, bytes_requested);
    }

    stream->cursor += bytes_requested;
    return bytes_requested;
}

static int lcd_jpeg_output(JDEC *decoder, void *bitmap, JRECT *rect)
{
    const lcd_jpeg_stream_t *stream = (const lcd_jpeg_stream_t *)decoder->device;
    const uint16_t w = (uint16_t)(rect->right - rect->left + 1u);
    const uint16_t h = (uint16_t)(rect->bottom - rect->top + 1u);
    const uint8_t *src = (const uint8_t *)bitmap;
    uint8_t wire[64 * 3];

    if (!lcd_begin_write(stream->drv,
                         (uint16_t)(stream->origin_x + rect->left),
                         (uint16_t)(stream->origin_y + rect->top),
                         w, h)) {
        return 1;
    }

    {
        size_t remaining = (size_t)w * h;

        while (remaining > 0) {
            const size_t chunk = remaining > 64u ? 64u : remaining;

            for (size_t i = 0; i < chunk; ++i) {
                wire[i * 3u + 0] = (uint8_t)(src[0] & 0xFCu);
                wire[i * 3u + 1] = (uint8_t)(src[1] & 0xFCu);
                wire[i * 3u + 2] = (uint8_t)(src[2] & 0xFCu);
                src += 3;
            }

            lcd_write_pixels(stream->drv, wire, chunk);
            remaining -= chunk;
        }
    }

    lcd_flush(stream->drv);
    return 1;
}

static bool lcd_jpeg_prepare(
    const uint8_t *jpeg_data,
    size_t jpeg_size,
    JDEC *decoder,
    lcd_jpeg_workspace_t *workspace,
    lcd_jpeg_stream_t *stream)
{
    if (jpeg_data == NULL || jpeg_size == 0u || decoder == NULL || workspace == NULL || stream == NULL) {
        return false;
    }

    stream->cursor = jpeg_data;
    stream->end = jpeg_data + jpeg_size;
    stream->drv = NULL;
    stream->origin_x = 0u;
    stream->origin_y = 0u;

    return jd_prepare(decoder, lcd_jpeg_input, workspace->bytes, sizeof(workspace->bytes), stream) == JDR_OK;
}

bool lcd_jpeg_get_info(const uint8_t *jpeg_data, size_t jpeg_size, lcd_jpeg_info_t *out_info)
{
    lcd_jpeg_stream_t stream;
    JDEC decoder;

    if (!lcd_jpeg_prepare(jpeg_data, jpeg_size, &decoder, &g_lcd_jpeg_workspace, &stream)) {
        return false;
    }

    if (out_info != NULL) {
        out_info->width = decoder.width;
        out_info->height = decoder.height;
    }

    return true;
}

bool lcd_jpeg_draw(lcd_driver_t *drv, const uint8_t *jpeg_data, size_t jpeg_size, uint16_t x, uint16_t y)
{
    lcd_jpeg_stream_t stream;
    JDEC decoder;

    if (drv == NULL) {
        return false;
    }

    if (!lcd_jpeg_prepare(jpeg_data, jpeg_size, &decoder, &g_lcd_jpeg_workspace, &stream)) {
        return false;
    }

    if ((uint32_t)x + decoder.width > lcd_width(drv) || (uint32_t)y + decoder.height > lcd_height(drv)) {
        return false;
    }

    stream.drv = drv;
    stream.origin_x = x;
    stream.origin_y = y;
    return jd_decomp(&decoder, lcd_jpeg_output, 0) == JDR_OK;
}
