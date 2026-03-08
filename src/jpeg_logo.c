#include "jpeg_logo.h"

#include "ili9486l.h"
#include "tjpgd/tjpgd.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define LOGO_JPEG_WORKBUF_SIZE 4096u

extern const uint8_t g_logo_jpg_start[];
extern const uint8_t g_logo_jpg_end[];

typedef struct {
  const uint8_t *cursor;
  const uint8_t *end;
} jpeg_logo_stream_t;

typedef union {
  uint32_t align;
  uint8_t bytes[LOGO_JPEG_WORKBUF_SIZE];
} jpeg_logo_workspace_t;

static size_t jpeg_logo_input(JDEC *jd, uint8_t *buffer, size_t bytes_requested) {
  jpeg_logo_stream_t *stream = (jpeg_logo_stream_t *)jd->device;
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

static int jpeg_logo_output(JDEC *jd, void *bitmap, JRECT *rect) {
  (void)jd;

  ili9486l_draw_rgb888_as_rgb666_rect(
      (const uint8_t *)bitmap,
      rect->left,
      rect->top,
      (uint16_t)(rect->right - rect->left + 1u),
      (uint16_t)(rect->bottom - rect->top + 1u));
  return 1;
}

bool jpeg_logo_show(void) {
  static jpeg_logo_workspace_t workspace;
  jpeg_logo_stream_t stream = {
      .cursor = g_logo_jpg_start,
      .end = g_logo_jpg_end,
  };
  JDEC decoder;
  JRESULT result;

  if (stream.cursor == stream.end) {
    return false;
  }

  result = jd_prepare(&decoder, jpeg_logo_input, workspace.bytes, sizeof(workspace.bytes), &stream);
  if (result != JDR_OK) {
    return false;
  }

  if (decoder.width != ili9486l_width() || decoder.height != ili9486l_height()) {
    return false;
  }

  result = jd_decomp(&decoder, jpeg_logo_output, 0);
  return result == JDR_OK;
}
