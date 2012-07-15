#pragma once

#ifdef DEBUG
//#include "utils/uartstdio.h"
#include "screen.h"

inline void dump_hex_bytes(uint8_t *p, unsigned int len) {
  if (len) printf("  ");

  for (unsigned int i=0; i < len; ++i) {
    printf("%02x ", *p++);
    if (i > 15 && (0xf & i) == 0) printf("\n  ");
  }
}
#endif
