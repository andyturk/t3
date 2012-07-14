#pragma once

#include <stdint.h>

extern const char hex_digits[16];

struct BD_ADDR {
  static char pp_buf[24];
  uint8_t data[6];

  const char *pretty_print() {
    char *p = pp_buf;

    for (int i=5; i >= 0; --i) {
      *p++ = hex_digits[data[i] >> 4];
      *p++ = hex_digits[data[i] & 0x0f];
      if (i > 0) *p++ = ':';
    }
    *p++ = 0;

    return pp_buf;
  }
};
