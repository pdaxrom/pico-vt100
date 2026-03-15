#ifndef FONT5X7_H
#define FONT5X7_H

#include <stdint.h>

#define FONT5X7_CELL6X9_HEIGHT 9u
#define FONT5X7_FIRST_CHAR 0x20u
#define FONT5X7_LAST_CHAR 0x7Fu

const uint8_t *font5x7_get_cell6x9_row_masks(char c);

#endif
