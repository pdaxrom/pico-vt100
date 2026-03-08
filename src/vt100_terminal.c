#include "vt100_terminal.h"

#include "font5x7.h"
#include "ili9486l.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

enum {
  VT100_STATE_GROUND = 0,
  VT100_STATE_ESC,
  VT100_STATE_CSI,
};

#define VT100_ATTR(fg, bg) (uint8_t)((((bg) & 0x0Fu) << 4) | ((fg) & 0x0Fu))
#define VT100_ATTR_FG(attr) (uint8_t)((attr) & 0x0Fu)
#define VT100_ATTR_BG(attr) (uint8_t)(((attr) >> 4) & 0x0Fu)

static const uint8_t k_vt100_palette[16][3] = {
  {0u, 0u, 0u},
  {42u, 0u, 0u},
  {0u, 42u, 0u},
  {42u, 42u, 0u},
  {0u, 0u, 42u},
  {42u, 0u, 42u},
  {0u, 42u, 42u},
  {48u, 48u, 48u},
  {24u, 24u, 24u},
  {63u, 0u, 0u},
  {0u, 63u, 0u},
  {63u, 63u, 0u},
  {0u, 0u, 63u},
  {63u, 0u, 63u},
  {0u, 63u, 63u},
  {63u, 63u, 63u},
};

static uint8_t g_row_buffer[VT100_TERMINAL_WIDTH_PIXELS * VT100_TERMINAL_CELL_HEIGHT * 3u];

static uint8_t vt100_terminal_current_attr(const vt100_terminal_t *terminal) {
  return VT100_ATTR(terminal->fg, terminal->bg);
}

static uint8_t vt100_terminal_sanitize_char(char ch) {
  if ((unsigned char)ch < 0x20u || (unsigned char)ch > 0x7Eu) {
    return '?';
  }

  return (uint8_t)ch;
}

static void vt100_terminal_set_cell(vt100_terminal_t *terminal, uint8_t row, uint8_t col, char ch, uint8_t attr) {
  terminal->cells[row][col].ch = ch;
  terminal->cells[row][col].attr = attr;
}

static void vt100_terminal_fill_range(vt100_terminal_t *terminal, uint8_t row, uint8_t col_start, uint8_t col_end, uint8_t attr) {
  for (uint8_t col = col_start; col <= col_end; ++col) {
    vt100_terminal_set_cell(terminal, row, col, ' ', attr);
  }
}

static bool vt100_terminal_glyph_pixel_on(const uint8_t glyph_rows[VT100_TERMINAL_GLYPH_HEIGHT], uint8_t px, uint8_t py) {
  if (px >= VT100_TERMINAL_GLYPH_WIDTH) {
    return false;
  }

  if (py < VT100_TERMINAL_GLYPH_Y_OFFSET) {
    return false;
  }

  py = (uint8_t)(py - VT100_TERMINAL_GLYPH_Y_OFFSET);
  if (py >= VT100_TERMINAL_GLYPH_HEIGHT) {
    return false;
  }

  return (glyph_rows[py] & (1u << (VT100_TERMINAL_GLYPH_WIDTH - 1u - px))) != 0u;
}

static void vt100_terminal_render_cell_internal(const vt100_terminal_t *terminal, uint8_t row, uint8_t col, bool invert) {
  uint8_t cell_pixels[VT100_TERMINAL_CELL_WIDTH * VT100_TERMINAL_CELL_HEIGHT * 3u];
  uint8_t glyph_rows[VT100_TERMINAL_GLYPH_HEIGHT];
  const vt100_terminal_cell_t *cell = &terminal->cells[row][col];
  uint8_t fg = VT100_ATTR_FG(cell->attr);
  uint8_t bg = VT100_ATTR_BG(cell->attr);

  if (invert) {
    const uint8_t tmp = fg;
    fg = bg;
    bg = tmp;
  }

  font5x7_get_rows((char)vt100_terminal_sanitize_char(cell->ch), glyph_rows);

  for (uint8_t py = 0; py < VT100_TERMINAL_CELL_HEIGHT; ++py) {
    for (uint8_t px = 0; px < VT100_TERMINAL_CELL_WIDTH; ++px) {
      const bool pixel_on = vt100_terminal_glyph_pixel_on(glyph_rows, px, py);
      const uint8_t *color = k_vt100_palette[pixel_on ? fg : bg];
      uint8_t *dst = &cell_pixels[(py * VT100_TERMINAL_CELL_WIDTH + px) * 3u];

      dst[0] = color[0];
      dst[1] = color[1];
      dst[2] = color[2];
    }
  }

  ili9486l_draw_rgb666_rect(
      cell_pixels,
      (uint16_t)(terminal->origin_x + col * VT100_TERMINAL_CELL_WIDTH),
      (uint16_t)(terminal->origin_y + row * VT100_TERMINAL_CELL_HEIGHT),
      VT100_TERMINAL_CELL_WIDTH,
      VT100_TERMINAL_CELL_HEIGHT);
}

static void vt100_terminal_hide_cursor(vt100_terminal_t *terminal) {
  if (!terminal->cursor_visible) {
    return;
  }

  vt100_terminal_render_cell_internal(terminal, terminal->cursor_row, terminal->cursor_col, false);
}

static void vt100_terminal_show_cursor(vt100_terminal_t *terminal) {
  if (!terminal->cursor_visible) {
    return;
  }

  vt100_terminal_render_cell_internal(terminal, terminal->cursor_row, terminal->cursor_col, true);
}

static void vt100_terminal_render_row(vt100_terminal_t *terminal, uint8_t row) {
  for (uint8_t col = 0; col < VT100_TERMINAL_COLS; ++col) {
    uint8_t glyph_rows[VT100_TERMINAL_GLYPH_HEIGHT];
    const vt100_terminal_cell_t *cell = &terminal->cells[row][col];
    const uint8_t *fg = k_vt100_palette[VT100_ATTR_FG(cell->attr)];
    const uint8_t *bg = k_vt100_palette[VT100_ATTR_BG(cell->attr)];
    const uint16_t cell_x = (uint16_t)(col * VT100_TERMINAL_CELL_WIDTH);

    font5x7_get_rows((char)vt100_terminal_sanitize_char(cell->ch), glyph_rows);

    for (uint8_t py = 0; py < VT100_TERMINAL_CELL_HEIGHT; ++py) {
      for (uint8_t px = 0; px < VT100_TERMINAL_CELL_WIDTH; ++px) {
        const bool pixel_on = vt100_terminal_glyph_pixel_on(glyph_rows, px, py);
        const uint8_t *color = pixel_on ? fg : bg;
        uint8_t *dst = &g_row_buffer[((py * VT100_TERMINAL_WIDTH_PIXELS) + cell_x + px) * 3u];

        dst[0] = color[0];
        dst[1] = color[1];
        dst[2] = color[2];
      }
    }
  }

  ili9486l_draw_rgb666_rect(
      g_row_buffer,
      terminal->origin_x,
      (uint16_t)(terminal->origin_y + row * VT100_TERMINAL_CELL_HEIGHT),
      VT100_TERMINAL_WIDTH_PIXELS,
      VT100_TERMINAL_CELL_HEIGHT);
}

static void vt100_terminal_scroll_up(vt100_terminal_t *terminal) {
  memmove(
      &terminal->cells[0][0],
      &terminal->cells[1][0],
      (VT100_TERMINAL_ROWS - 1u) * VT100_TERMINAL_COLS * sizeof(vt100_terminal_cell_t));

  vt100_terminal_fill_range(
      terminal,
      (uint8_t)(VT100_TERMINAL_ROWS - 1u),
      0,
      (uint8_t)(VT100_TERMINAL_COLS - 1u),
      vt100_terminal_current_attr(terminal));

  for (uint8_t row = 0; row < VT100_TERMINAL_ROWS; ++row) {
    vt100_terminal_render_row(terminal, row);
  }
}

static void vt100_terminal_newline(vt100_terminal_t *terminal) {
  if (terminal->cursor_row == VT100_TERMINAL_ROWS - 1u) {
    vt100_terminal_scroll_up(terminal);
  } else {
    ++terminal->cursor_row;
  }
}

static void vt100_terminal_advance(vt100_terminal_t *terminal) {
  if (terminal->cursor_col == VT100_TERMINAL_COLS - 1u) {
    terminal->cursor_col = 0;
    vt100_terminal_newline(terminal);
  } else {
    ++terminal->cursor_col;
  }
}

static uint16_t vt100_terminal_param_or(const vt100_terminal_t *terminal, uint8_t index, uint16_t fallback) {
  if (index >= terminal->csi_param_count) {
    return fallback;
  }

  return terminal->csi_params[index] == 0u ? fallback : terminal->csi_params[index];
}

static void vt100_terminal_push_csi_value(vt100_terminal_t *terminal) {
  if (terminal->csi_param_count >= (sizeof(terminal->csi_params) / sizeof(terminal->csi_params[0]))) {
    return;
  }

  if (terminal->csi_have_value || terminal->csi_param_count > 0u) {
    terminal->csi_params[terminal->csi_param_count++] = terminal->csi_value;
  }

  terminal->csi_value = 0;
  terminal->csi_have_value = 0;
}

static void vt100_terminal_erase_display(vt100_terminal_t *terminal, uint16_t mode) {
  const uint8_t attr = vt100_terminal_current_attr(terminal);

  if (mode == 2u) {
    for (uint8_t row = 0; row < VT100_TERMINAL_ROWS; ++row) {
      vt100_terminal_fill_range(terminal, row, 0, (uint8_t)(VT100_TERMINAL_COLS - 1u), attr);
      vt100_terminal_render_row(terminal, row);
    }
    return;
  }

  if (mode == 0u) {
    vt100_terminal_fill_range(terminal, terminal->cursor_row, terminal->cursor_col, (uint8_t)(VT100_TERMINAL_COLS - 1u), attr);
    vt100_terminal_render_row(terminal, terminal->cursor_row);
    for (uint8_t row = (uint8_t)(terminal->cursor_row + 1u); row < VT100_TERMINAL_ROWS; ++row) {
      vt100_terminal_fill_range(terminal, row, 0, (uint8_t)(VT100_TERMINAL_COLS - 1u), attr);
      vt100_terminal_render_row(terminal, row);
    }
    return;
  }

  if (mode == 1u) {
    for (uint8_t row = 0; row < terminal->cursor_row; ++row) {
      vt100_terminal_fill_range(terminal, row, 0, (uint8_t)(VT100_TERMINAL_COLS - 1u), attr);
      vt100_terminal_render_row(terminal, row);
    }
    vt100_terminal_fill_range(terminal, terminal->cursor_row, 0, terminal->cursor_col, attr);
    vt100_terminal_render_row(terminal, terminal->cursor_row);
  }
}

static void vt100_terminal_erase_line(vt100_terminal_t *terminal, uint16_t mode) {
  const uint8_t attr = vt100_terminal_current_attr(terminal);

  if (mode == 2u) {
    vt100_terminal_fill_range(terminal, terminal->cursor_row, 0, (uint8_t)(VT100_TERMINAL_COLS - 1u), attr);
  } else if (mode == 1u) {
    vt100_terminal_fill_range(terminal, terminal->cursor_row, 0, terminal->cursor_col, attr);
  } else {
    vt100_terminal_fill_range(terminal, terminal->cursor_row, terminal->cursor_col, (uint8_t)(VT100_TERMINAL_COLS - 1u), attr);
  }

  vt100_terminal_render_row(terminal, terminal->cursor_row);
}

static void vt100_terminal_apply_sgr(vt100_terminal_t *terminal) {
  if (terminal->csi_param_count == 0u) {
    terminal->fg = terminal->default_fg;
    terminal->bg = terminal->default_bg;
    return;
  }

  for (uint8_t i = 0; i < terminal->csi_param_count; ++i) {
    const uint16_t param = terminal->csi_params[i];

    if (param == 0u) {
      terminal->fg = terminal->default_fg;
      terminal->bg = terminal->default_bg;
    } else if (param == 1u) {
      if (terminal->fg < 8u) {
        terminal->fg = (uint8_t)(terminal->fg + 8u);
      }
    } else if (param == 22u) {
      if (terminal->fg >= 8u) {
        terminal->fg = (uint8_t)(terminal->fg - 8u);
      }
    } else if (param >= 30u && param <= 37u) {
      terminal->fg = (uint8_t)(param - 30u);
    } else if (param == 39u) {
      terminal->fg = terminal->default_fg;
    } else if (param >= 40u && param <= 47u) {
      terminal->bg = (uint8_t)(param - 40u);
    } else if (param == 49u) {
      terminal->bg = terminal->default_bg;
    } else if (param >= 90u && param <= 97u) {
      terminal->fg = (uint8_t)(8u + (param - 90u));
    } else if (param >= 100u && param <= 107u) {
      terminal->bg = (uint8_t)(8u + (param - 100u));
    }
  }
}

static void vt100_terminal_dispatch_csi(vt100_terminal_t *terminal, char final_char) {
  const uint16_t first = vt100_terminal_param_or(terminal, 0, 1u);
  const uint16_t second = vt100_terminal_param_or(terminal, 1, 1u);

  if (terminal->csi_private) {
    return;
  }

  switch (final_char) {
    case 'A':
      if (terminal->cursor_row >= vt100_terminal_param_or(terminal, 0, 1u)) {
        terminal->cursor_row = (uint8_t)(terminal->cursor_row - vt100_terminal_param_or(terminal, 0, 1u));
      } else {
        terminal->cursor_row = 0;
      }
      break;
    case 'B': {
      const uint16_t next = (uint16_t)(terminal->cursor_row + vt100_terminal_param_or(terminal, 0, 1u));
      terminal->cursor_row = (uint8_t)(next >= VT100_TERMINAL_ROWS ? (VT100_TERMINAL_ROWS - 1u) : next);
      break;
    }
    case 'C': {
      const uint16_t next = (uint16_t)(terminal->cursor_col + vt100_terminal_param_or(terminal, 0, 1u));
      terminal->cursor_col = (uint8_t)(next >= VT100_TERMINAL_COLS ? (VT100_TERMINAL_COLS - 1u) : next);
      break;
    }
    case 'D':
      if (terminal->cursor_col >= vt100_terminal_param_or(terminal, 0, 1u)) {
        terminal->cursor_col = (uint8_t)(terminal->cursor_col - vt100_terminal_param_or(terminal, 0, 1u));
      } else {
        terminal->cursor_col = 0;
      }
      break;
    case 'G':
      terminal->cursor_col = (uint8_t)((first == 0u || first > VT100_TERMINAL_COLS) ? (VT100_TERMINAL_COLS - 1u) : (first - 1u));
      break;
    case 'H':
    case 'f':
      terminal->cursor_row = (uint8_t)((first == 0u || first > VT100_TERMINAL_ROWS) ? (VT100_TERMINAL_ROWS - 1u) : (first - 1u));
      terminal->cursor_col = (uint8_t)((second == 0u || second > VT100_TERMINAL_COLS) ? (VT100_TERMINAL_COLS - 1u) : (second - 1u));
      break;
    case 'J':
      vt100_terminal_erase_display(terminal, vt100_terminal_param_or(terminal, 0, 0u));
      break;
    case 'K':
      vt100_terminal_erase_line(terminal, vt100_terminal_param_or(terminal, 0, 0u));
      break;
    case 'd':
      terminal->cursor_row = (uint8_t)((first == 0u || first > VT100_TERMINAL_ROWS) ? (VT100_TERMINAL_ROWS - 1u) : (first - 1u));
      break;
    case 'm':
      vt100_terminal_apply_sgr(terminal);
      break;
    case 's':
      terminal->saved_row = terminal->cursor_row;
      terminal->saved_col = terminal->cursor_col;
      break;
    case 'u':
      terminal->cursor_row = terminal->saved_row;
      terminal->cursor_col = terminal->saved_col;
      break;
  }
}

void vt100_terminal_render(vt100_terminal_t *terminal) {
  vt100_terminal_hide_cursor(terminal);

  for (uint8_t row = 0; row < VT100_TERMINAL_ROWS; ++row) {
    vt100_terminal_render_row(terminal, row);
  }

  vt100_terminal_show_cursor(terminal);
}

void vt100_terminal_reset(vt100_terminal_t *terminal) {
  memset(terminal->cells, 0, sizeof(terminal->cells));

  terminal->cursor_row = 0;
  terminal->cursor_col = 0;
  terminal->saved_row = 0;
  terminal->saved_col = 0;
  terminal->default_fg = 15u;
  terminal->default_bg = 0u;
  terminal->fg = terminal->default_fg;
  terminal->bg = terminal->default_bg;
  terminal->state = VT100_STATE_GROUND;
  terminal->csi_param_count = 0;
  terminal->csi_have_value = 0;
  terminal->csi_private = 0;
  terminal->csi_value = 0;

  for (uint8_t row = 0; row < VT100_TERMINAL_ROWS; ++row) {
    vt100_terminal_fill_range(terminal, row, 0, (uint8_t)(VT100_TERMINAL_COLS - 1u), vt100_terminal_current_attr(terminal));
  }

  ili9486l_fill_rect(
      terminal->origin_x,
      terminal->origin_y,
      VT100_TERMINAL_WIDTH_PIXELS,
      VT100_TERMINAL_HEIGHT_PIXELS,
      LCD_COLOR_BLACK);

  vt100_terminal_render(terminal);
}

void vt100_terminal_init(vt100_terminal_t *terminal, uint16_t origin_x, uint16_t origin_y) {
  memset(terminal, 0, sizeof(*terminal));
  terminal->origin_x = origin_x;
  terminal->origin_y = origin_y;
  terminal->cursor_visible = true;
  vt100_terminal_reset(terminal);
}

void vt100_terminal_putc(vt100_terminal_t *terminal, char ch) {
  vt100_terminal_hide_cursor(terminal);

  switch (terminal->state) {
    case VT100_STATE_GROUND:
      if (ch == '\x1b') {
        terminal->state = VT100_STATE_ESC;
        break;
      }

      switch ((unsigned char)ch) {
        case '\a':
          break;
        case '\b':
          if (terminal->cursor_col > 0u) {
            --terminal->cursor_col;
          }
          break;
        case '\t': {
          const uint8_t next_tab = (uint8_t)(((terminal->cursor_col / 8u) + 1u) * 8u);
          terminal->cursor_col = next_tab >= VT100_TERMINAL_COLS ? (VT100_TERMINAL_COLS - 1u) : next_tab;
          break;
        }
        case '\n':
        case '\v':
        case '\f':
          vt100_terminal_newline(terminal);
          break;
        case '\r':
          terminal->cursor_col = 0;
          break;
        default:
          if ((unsigned char)ch >= 0x20u) {
            vt100_terminal_set_cell(terminal, terminal->cursor_row, terminal->cursor_col, ch, vt100_terminal_current_attr(terminal));
            vt100_terminal_render_cell_internal(terminal, terminal->cursor_row, terminal->cursor_col, false);
            vt100_terminal_advance(terminal);
          }
          break;
      }
      break;

    case VT100_STATE_ESC:
      terminal->state = VT100_STATE_GROUND;
      if (ch == '[') {
        terminal->state = VT100_STATE_CSI;
        terminal->csi_param_count = 0;
        terminal->csi_have_value = 0;
        terminal->csi_private = 0;
        terminal->csi_value = 0;
      } else if (ch == '7') {
        terminal->saved_row = terminal->cursor_row;
        terminal->saved_col = terminal->cursor_col;
      } else if (ch == '8') {
        terminal->cursor_row = terminal->saved_row;
        terminal->cursor_col = terminal->saved_col;
      } else if (ch == 'c') {
        vt100_terminal_reset(terminal);
        return;
      } else if (ch == 'D') {
        vt100_terminal_newline(terminal);
      } else if (ch == 'E') {
        terminal->cursor_col = 0;
        vt100_terminal_newline(terminal);
      }
      break;

    case VT100_STATE_CSI:
      if (ch >= '0' && ch <= '9') {
        terminal->csi_value = (uint16_t)(terminal->csi_value * 10u + (uint16_t)(ch - '0'));
        terminal->csi_have_value = 1;
      } else if (ch == ';') {
        vt100_terminal_push_csi_value(terminal);
      } else if (ch == '?') {
        terminal->csi_private = 1;
      } else {
        vt100_terminal_push_csi_value(terminal);
        vt100_terminal_dispatch_csi(terminal, ch);
        terminal->state = VT100_STATE_GROUND;
      }
      break;
  }

  vt100_terminal_show_cursor(terminal);
}

void vt100_terminal_write(vt100_terminal_t *terminal, const char *text) {
  if (text == NULL) {
    return;
  }

  while (*text != '\0') {
    vt100_terminal_putc(terminal, *text++);
  }
}
