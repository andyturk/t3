#pragma once

#include <stdint.h>

extern const char hex_digits[16];

struct BD_ADDR {
  enum {PP_BUF_LEN = 24};
  uint8_t data[6];
  void pretty_print(char *buf) {
    for (int i=5; i >= 0; --i) {
      *buf++ = hex_digits[data[i] >> 4];
      *buf++ = hex_digits[data[i] & 0x0f];
      if (i > 0) *buf++ = ':';
    }
    *buf++ = 0;
  }
};
