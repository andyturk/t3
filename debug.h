#pragma once

#ifdef DEBUG
#include "utils/uartstdio.h"

inline void dump_hex_bytes(uint8_t *p, unsigned int len) {
  while (len--) UARTprintf("%02x ", *p++);
}
#endif
