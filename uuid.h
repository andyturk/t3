#pragma once

#include <stdint.h>

class UUID {
  uint8_t data[16];
  UUID();

  static UUID base;

 public:
  UUID(const char *s); // e.g., UUID("00001234-0000-1000-8000-00805F9B34FB")
  UUID(uint16_t shortened);
  UUID(const UUID &other);

  operator uint16_t() { return data[3] + (data[2] << 8); }
  static int compare(const UUID &u1, const UUID &u2);
};

inline bool operator==(const UUID &l, const UUID &r) { return UUID::compare(l, r) == 0; }
inline bool operator!=(const UUID &l, const UUID &r) { return UUID::compare(l, r) != 0; }
