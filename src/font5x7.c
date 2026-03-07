#include "font5x7.h"

#include <string.h>

void font5x7_get_rows(char c, uint8_t rows[7]) {
  memset(rows, 0, 7);

  if (c >= 'a' && c <= 'z') {
    c = (char)(c - 'a' + 'A');
  }

  switch (c) {
    case ' ':
      return;
    case '!':
      rows[0] = 0x04;
      rows[1] = 0x04;
      rows[2] = 0x04;
      rows[3] = 0x04;
      rows[4] = 0x04;
      rows[6] = 0x04;
      return;
    case '"':
      rows[0] = 0x0A;
      rows[1] = 0x0A;
      rows[2] = 0x0A;
      return;
    case '#':
      rows[0] = 0x0A;
      rows[1] = 0x1F;
      rows[2] = 0x0A;
      rows[3] = 0x0A;
      rows[4] = 0x1F;
      rows[5] = 0x0A;
      return;
    case '\'':
      rows[0] = 0x04;
      rows[1] = 0x04;
      rows[2] = 0x08;
      return;
    case '(':
    case '[':
      rows[0] = 0x02;
      rows[1] = 0x04;
      rows[2] = 0x08;
      rows[3] = 0x08;
      rows[4] = 0x08;
      rows[5] = 0x04;
      rows[6] = 0x02;
      return;
    case ')':
    case ']':
      rows[0] = 0x08;
      rows[1] = 0x04;
      rows[2] = 0x02;
      rows[3] = 0x02;
      rows[4] = 0x02;
      rows[5] = 0x04;
      rows[6] = 0x08;
      return;
    case '*':
      rows[1] = 0x15;
      rows[2] = 0x0E;
      rows[3] = 0x1F;
      rows[4] = 0x0E;
      rows[5] = 0x15;
      return;
    case '+':
      rows[1] = 0x04;
      rows[2] = 0x04;
      rows[3] = 0x1F;
      rows[4] = 0x04;
      rows[5] = 0x04;
      return;
    case ',':
      rows[4] = 0x04;
      rows[5] = 0x04;
      rows[6] = 0x08;
      return;
    case '-':
      rows[3] = 0x1F;
      return;
    case '.':
      rows[5] = 0x0C;
      rows[6] = 0x0C;
      return;
    case '/':
      rows[0] = 0x01;
      rows[1] = 0x02;
      rows[2] = 0x04;
      rows[3] = 0x08;
      rows[4] = 0x10;
      return;
    case '0':
      rows[0] = 0x0E;
      rows[1] = 0x11;
      rows[2] = 0x13;
      rows[3] = 0x15;
      rows[4] = 0x19;
      rows[5] = 0x11;
      rows[6] = 0x0E;
      return;
    case '1':
      rows[0] = 0x04;
      rows[1] = 0x0C;
      rows[2] = 0x04;
      rows[3] = 0x04;
      rows[4] = 0x04;
      rows[5] = 0x04;
      rows[6] = 0x0E;
      return;
    case '2':
      rows[0] = 0x0E;
      rows[1] = 0x11;
      rows[2] = 0x01;
      rows[3] = 0x02;
      rows[4] = 0x04;
      rows[5] = 0x08;
      rows[6] = 0x1F;
      return;
    case '3':
      rows[0] = 0x1E;
      rows[1] = 0x01;
      rows[2] = 0x01;
      rows[3] = 0x0E;
      rows[4] = 0x01;
      rows[5] = 0x01;
      rows[6] = 0x1E;
      return;
    case '4':
      rows[0] = 0x02;
      rows[1] = 0x06;
      rows[2] = 0x0A;
      rows[3] = 0x12;
      rows[4] = 0x1F;
      rows[5] = 0x02;
      rows[6] = 0x02;
      return;
    case '5':
      rows[0] = 0x1F;
      rows[1] = 0x10;
      rows[2] = 0x10;
      rows[3] = 0x1E;
      rows[4] = 0x01;
      rows[5] = 0x01;
      rows[6] = 0x1E;
      return;
    case '6':
      rows[0] = 0x0E;
      rows[1] = 0x10;
      rows[2] = 0x10;
      rows[3] = 0x1E;
      rows[4] = 0x11;
      rows[5] = 0x11;
      rows[6] = 0x0E;
      return;
    case '7':
      rows[0] = 0x1F;
      rows[1] = 0x01;
      rows[2] = 0x02;
      rows[3] = 0x04;
      rows[4] = 0x08;
      rows[5] = 0x08;
      rows[6] = 0x08;
      return;
    case '8':
      rows[0] = 0x0E;
      rows[1] = 0x11;
      rows[2] = 0x11;
      rows[3] = 0x0E;
      rows[4] = 0x11;
      rows[5] = 0x11;
      rows[6] = 0x0E;
      return;
    case '9':
      rows[0] = 0x0E;
      rows[1] = 0x11;
      rows[2] = 0x11;
      rows[3] = 0x0F;
      rows[4] = 0x01;
      rows[5] = 0x01;
      rows[6] = 0x0E;
      return;
    case ':':
      rows[1] = 0x0C;
      rows[2] = 0x0C;
      rows[4] = 0x0C;
      rows[5] = 0x0C;
      return;
    case ';':
      rows[1] = 0x0C;
      rows[2] = 0x0C;
      rows[4] = 0x0C;
      rows[5] = 0x0C;
      rows[6] = 0x08;
      return;
    case '<':
      rows[0] = 0x02;
      rows[1] = 0x04;
      rows[2] = 0x08;
      rows[3] = 0x10;
      rows[4] = 0x08;
      rows[5] = 0x04;
      rows[6] = 0x02;
      return;
    case '=':
      rows[2] = 0x1F;
      rows[4] = 0x1F;
      return;
    case '>':
      rows[0] = 0x08;
      rows[1] = 0x04;
      rows[2] = 0x02;
      rows[3] = 0x01;
      rows[4] = 0x02;
      rows[5] = 0x04;
      rows[6] = 0x08;
      return;
    case '?':
      rows[0] = 0x0E;
      rows[1] = 0x11;
      rows[2] = 0x01;
      rows[3] = 0x02;
      rows[4] = 0x04;
      rows[6] = 0x04;
      return;
    case '\\':
      rows[0] = 0x10;
      rows[1] = 0x08;
      rows[2] = 0x04;
      rows[3] = 0x02;
      rows[4] = 0x01;
      return;
    case '^':
      rows[0] = 0x04;
      rows[1] = 0x0A;
      rows[2] = 0x11;
      return;
    case '_':
      rows[6] = 0x1F;
      return;
    case 'A':
      rows[0] = 0x0E;
      rows[1] = 0x11;
      rows[2] = 0x11;
      rows[3] = 0x1F;
      rows[4] = 0x11;
      rows[5] = 0x11;
      rows[6] = 0x11;
      return;
    case 'B':
      rows[0] = 0x1E;
      rows[1] = 0x11;
      rows[2] = 0x11;
      rows[3] = 0x1E;
      rows[4] = 0x11;
      rows[5] = 0x11;
      rows[6] = 0x1E;
      return;
    case 'C':
      rows[0] = 0x0E;
      rows[1] = 0x11;
      rows[2] = 0x10;
      rows[3] = 0x10;
      rows[4] = 0x10;
      rows[5] = 0x11;
      rows[6] = 0x0E;
      return;
    case 'D':
      rows[0] = 0x1E;
      rows[1] = 0x11;
      rows[2] = 0x11;
      rows[3] = 0x11;
      rows[4] = 0x11;
      rows[5] = 0x11;
      rows[6] = 0x1E;
      return;
    case 'E':
      rows[0] = 0x1F;
      rows[1] = 0x10;
      rows[2] = 0x10;
      rows[3] = 0x1E;
      rows[4] = 0x10;
      rows[5] = 0x10;
      rows[6] = 0x1F;
      return;
    case 'F':
      rows[0] = 0x1F;
      rows[1] = 0x10;
      rows[2] = 0x10;
      rows[3] = 0x1E;
      rows[4] = 0x10;
      rows[5] = 0x10;
      rows[6] = 0x10;
      return;
    case 'G':
      rows[0] = 0x0E;
      rows[1] = 0x11;
      rows[2] = 0x10;
      rows[3] = 0x17;
      rows[4] = 0x11;
      rows[5] = 0x11;
      rows[6] = 0x0E;
      return;
    case 'H':
      rows[0] = 0x11;
      rows[1] = 0x11;
      rows[2] = 0x11;
      rows[3] = 0x1F;
      rows[4] = 0x11;
      rows[5] = 0x11;
      rows[6] = 0x11;
      return;
    case 'I':
      rows[0] = 0x0E;
      rows[1] = 0x04;
      rows[2] = 0x04;
      rows[3] = 0x04;
      rows[4] = 0x04;
      rows[5] = 0x04;
      rows[6] = 0x0E;
      return;
    case 'J':
      rows[0] = 0x01;
      rows[1] = 0x01;
      rows[2] = 0x01;
      rows[3] = 0x01;
      rows[4] = 0x11;
      rows[5] = 0x11;
      rows[6] = 0x0E;
      return;
    case 'K':
      rows[0] = 0x11;
      rows[1] = 0x12;
      rows[2] = 0x14;
      rows[3] = 0x18;
      rows[4] = 0x14;
      rows[5] = 0x12;
      rows[6] = 0x11;
      return;
    case 'L':
      rows[0] = 0x10;
      rows[1] = 0x10;
      rows[2] = 0x10;
      rows[3] = 0x10;
      rows[4] = 0x10;
      rows[5] = 0x10;
      rows[6] = 0x1F;
      return;
    case 'M':
      rows[0] = 0x11;
      rows[1] = 0x1B;
      rows[2] = 0x15;
      rows[3] = 0x15;
      rows[4] = 0x11;
      rows[5] = 0x11;
      rows[6] = 0x11;
      return;
    case 'N':
      rows[0] = 0x11;
      rows[1] = 0x19;
      rows[2] = 0x15;
      rows[3] = 0x13;
      rows[4] = 0x11;
      rows[5] = 0x11;
      rows[6] = 0x11;
      return;
    case 'O':
      rows[0] = 0x0E;
      rows[1] = 0x11;
      rows[2] = 0x11;
      rows[3] = 0x11;
      rows[4] = 0x11;
      rows[5] = 0x11;
      rows[6] = 0x0E;
      return;
    case 'P':
      rows[0] = 0x1E;
      rows[1] = 0x11;
      rows[2] = 0x11;
      rows[3] = 0x1E;
      rows[4] = 0x10;
      rows[5] = 0x10;
      rows[6] = 0x10;
      return;
    case 'Q':
      rows[0] = 0x0E;
      rows[1] = 0x11;
      rows[2] = 0x11;
      rows[3] = 0x11;
      rows[4] = 0x15;
      rows[5] = 0x12;
      rows[6] = 0x0D;
      return;
    case 'R':
      rows[0] = 0x1E;
      rows[1] = 0x11;
      rows[2] = 0x11;
      rows[3] = 0x1E;
      rows[4] = 0x14;
      rows[5] = 0x12;
      rows[6] = 0x11;
      return;
    case 'S':
      rows[0] = 0x0F;
      rows[1] = 0x10;
      rows[2] = 0x10;
      rows[3] = 0x0E;
      rows[4] = 0x01;
      rows[5] = 0x01;
      rows[6] = 0x1E;
      return;
    case 'T':
      rows[0] = 0x1F;
      rows[1] = 0x04;
      rows[2] = 0x04;
      rows[3] = 0x04;
      rows[4] = 0x04;
      rows[5] = 0x04;
      rows[6] = 0x04;
      return;
    case 'U':
      rows[0] = 0x11;
      rows[1] = 0x11;
      rows[2] = 0x11;
      rows[3] = 0x11;
      rows[4] = 0x11;
      rows[5] = 0x11;
      rows[6] = 0x0E;
      return;
    case 'V':
      rows[0] = 0x11;
      rows[1] = 0x11;
      rows[2] = 0x11;
      rows[3] = 0x11;
      rows[4] = 0x11;
      rows[5] = 0x0A;
      rows[6] = 0x04;
      return;
    case 'W':
      rows[0] = 0x11;
      rows[1] = 0x11;
      rows[2] = 0x11;
      rows[3] = 0x15;
      rows[4] = 0x15;
      rows[5] = 0x15;
      rows[6] = 0x0A;
      return;
    case 'X':
      rows[0] = 0x11;
      rows[1] = 0x11;
      rows[2] = 0x0A;
      rows[3] = 0x04;
      rows[4] = 0x0A;
      rows[5] = 0x11;
      rows[6] = 0x11;
      return;
    case 'Y':
      rows[0] = 0x11;
      rows[1] = 0x11;
      rows[2] = 0x0A;
      rows[3] = 0x04;
      rows[4] = 0x04;
      rows[5] = 0x04;
      rows[6] = 0x04;
      return;
    case 'Z':
      rows[0] = 0x1F;
      rows[1] = 0x01;
      rows[2] = 0x02;
      rows[3] = 0x04;
      rows[4] = 0x08;
      rows[5] = 0x10;
      rows[6] = 0x1F;
      return;
    default:
      if (c != '?') {
        font5x7_get_rows('?', rows);
      }
      return;
  }
}
