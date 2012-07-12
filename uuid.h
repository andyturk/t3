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

  bool is_16bit() const;
  operator uint16_t() const { return data[14] + (data[13] << 8); }
  operator uint8_t const *() const { return data; }
  bool operator==(uint16_t type) {return is_16bit() && (type == (uint16_t) *this);}
  bool operator!=(uint16_t type) {return !(*this == type);}

  static int compare(const UUID &u1, const UUID &u2);
};

inline bool operator==(const UUID &l, const UUID &r) { return UUID::compare(l, r) == 0; }
inline bool operator!=(const UUID &l, const UUID &r) { return UUID::compare(l, r) != 0; }
