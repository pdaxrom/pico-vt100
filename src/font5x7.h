#ifndef FONT5X7_H
#define FONT5X7_H

#include <stdint.h>

#define FONT5X7_WIDTH 5u
#define FONT5X7_HEIGHT 7u
#define FONT5X7_FIRST_CHAR 0x20u
#define FONT5X7_LAST_CHAR 0x7Fu

/* Sourced from andygock/glcd fonts/font5x7.h (Pascal Stang 5x7 ASCII table). */
extern const uint8_t Font5x7[(FONT5X7_LAST_CHAR - FONT5X7_FIRST_CHAR + 1u) * FONT5X7_WIDTH];

const uint8_t *font5x7_get_glyph(char c);

#endif
