#include "vt100_terminal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
  TEST_CHARSET_US = 0,
  TEST_CHARSET_UK = 1,
  TEST_CHARSET_DEC_SPECIAL = 2,
};

typedef struct {
  char data[128];
  size_t len;
} output_buffer_t;

static void fail_check(const char *expr, const char *file, int line) {
  fprintf(stderr, "%s:%d: check failed: %s\n", file, line, expr);
}

#define CHECK(expr)                  \
  do {                               \
    if (!(expr)) {                   \
      fail_check(#expr, __FILE__, __LINE__); \
      return 1;                      \
    }                                \
  } while (0)

static void output_capture(const char *data, size_t len, void *user_data) {
  output_buffer_t *buffer = (output_buffer_t *)user_data;

  if (buffer->len + len > sizeof(buffer->data)) {
    len = sizeof(buffer->data) - buffer->len;
  }

  memcpy(&buffer->data[buffer->len], data, len);
  buffer->len += len;
}

static void feed(vt100_terminal_t *terminal, const char *text) {
  while (*text != '\0') {
    vt100_terminal_putc(terminal, *text++);
  }
}

static int test_osc_is_skipped(void) {
  vt100_terminal_t terminal;

  vt100_terminal_init(&terminal, 0u, 0u);
  feed(&terminal, "A\x1b]0;ignored\x07" "B");

  CHECK(terminal.cells[0][0].ch == 'A');
  CHECK(terminal.cells[0][1].ch == 'B');
  CHECK(terminal.cursor_row == 0u);
  CHECK(terminal.cursor_col == 2u);
  return 0;
}

static int test_string_controls_are_skipped(void) {
  vt100_terminal_t terminal;

  vt100_terminal_init(&terminal, 0u, 0u);
  feed(&terminal, "A\x1bPdcs\x1b\\B\x1b^pm\x1b\\C\x1b_apc\x1b\\D\x1bXsos\x1b\\E");

  CHECK(terminal.cells[0][0].ch == 'A');
  CHECK(terminal.cells[0][1].ch == 'B');
  CHECK(terminal.cells[0][2].ch == 'C');
  CHECK(terminal.cells[0][3].ch == 'D');
  CHECK(terminal.cells[0][4].ch == 'E');
  CHECK(terminal.cursor_col == 5u);
  return 0;
}

static int test_can_and_sub_cancel_sequences(void) {
  vt100_terminal_t terminal;

  vt100_terminal_init(&terminal, 0u, 0u);
  feed(&terminal, "A\x1b[31");
  vt100_terminal_putc(&terminal, '\x18');
  feed(&terminal, "B\x1b]title");
  vt100_terminal_putc(&terminal, '\x1a');
  feed(&terminal, "C");

  CHECK(terminal.cells[0][0].ch == 'A');
  CHECK(terminal.cells[0][1].ch == 'B');
  CHECK(terminal.cells[0][2].ch == 'C');
  CHECK(terminal.cells[0][1].attr == 0x0Fu);
  CHECK(terminal.cells[0][2].attr == 0x0Fu);
  CHECK(terminal.cursor_col == 3u);
  return 0;
}

static int test_decom_cpr_is_relative(void) {
  vt100_terminal_t terminal;
  output_buffer_t output = {{0}, 0u};

  vt100_terminal_init(&terminal, 0u, 0u);
  vt100_terminal_set_output(&terminal, output_capture, &output);
  feed(&terminal, "\x1b[5;10r\x1b[?6h\x1b[3;4H\x1b[6n");

  CHECK(output.len == strlen("\x1b[3;4R"));
  CHECK(memcmp(output.data, "\x1b[3;4R", output.len) == 0);
  CHECK(terminal.cursor_row == 6u);
  CHECK(terminal.cursor_col == 3u);
  return 0;
}

static int test_vt52_cursoring_and_exit(void) {
  vt100_terminal_t terminal;

  vt100_terminal_init(&terminal, 0u, 0u);
  feed(&terminal, "\x1b[?2l\x1bY\x22\x24Z");

  CHECK(terminal.vt52_mode);
  CHECK(terminal.cells[2][4].ch == 'Z');

  feed(&terminal, "\x1b<\x1b[2;3HQ");

  CHECK(!terminal.vt52_mode);
  CHECK(terminal.cells[1][2].ch == 'Q');
  return 0;
}

static int test_single_shift_uses_g2_and_g3_once(void) {
  vt100_terminal_t terminal;

  vt100_terminal_init(&terminal, 0u, 0u);
  feed(&terminal, "\x1b*0\x1b+A\x1bNq\x1bO#x");

  CHECK(terminal.cells[0][0].ch == 'q');
  CHECK(terminal.cells[0][0].charset == TEST_CHARSET_DEC_SPECIAL);
  CHECK(terminal.cells[0][1].ch == '#');
  CHECK(terminal.cells[0][1].charset == TEST_CHARSET_UK);
  CHECK(terminal.cells[0][2].ch == 'x');
  CHECK(terminal.cells[0][2].charset == TEST_CHARSET_US);
  CHECK(!terminal.single_shift_pending);
  return 0;
}

static int test_decsc_decrc_restore_g2_g3(void) {
  vt100_terminal_t terminal;

  vt100_terminal_init(&terminal, 0u, 0u);
  feed(&terminal, "\x1b*0\x1b+A\x1b" "7\x1b*B\x1b+0\x1b" "8\x1bNq\x1bO#");

  CHECK(terminal.cells[0][0].ch == 'q');
  CHECK(terminal.cells[0][0].charset == TEST_CHARSET_DEC_SPECIAL);
  CHECK(terminal.cells[0][1].ch == '#');
  CHECK(terminal.cells[0][1].charset == TEST_CHARSET_UK);
  return 0;
}

int main(void) {
  static const struct {
    const char *name;
    int (*fn)(void);
  } tests[] = {
      {"osc_is_skipped", test_osc_is_skipped},
      {"string_controls_are_skipped", test_string_controls_are_skipped},
      {"can_and_sub_cancel_sequences", test_can_and_sub_cancel_sequences},
      {"decom_cpr_is_relative", test_decom_cpr_is_relative},
      {"vt52_cursoring_and_exit", test_vt52_cursoring_and_exit},
      {"single_shift_uses_g2_and_g3_once", test_single_shift_uses_g2_and_g3_once},
      {"decsc_decrc_restore_g2_g3", test_decsc_decrc_restore_g2_g3},
  };

  for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); ++i) {
    if (tests[i].fn() != 0) {
      fprintf(stderr, "test failed: %s\n", tests[i].name);
      return 1;
    }
  }

  puts("all host VT100 tests passed");
  return 0;
}
