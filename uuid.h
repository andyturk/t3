#pragma once

#include <stdint.h>

class UUID {
  uint8_t data[16];
  UUID();

  static UUID base;

 public:
  UUID(const char *s);
  UUID(uint16_t shortened);
  UUID(const UUID &other);

  static int compare(const UUID &u1, const UUID &u2);
};

inline bool operator==(const UUID &l, const UUID &r) { return UUID::compare(l, r) == 0; }
inline bool operator!=(const UUID &l, const UUID &r) { return UUID::compare(l, r) != 0; }
