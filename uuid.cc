#include "uuid.h"

// the byte order here needs to be reversed
UUID::UUID() :
  data({0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x10, 0x00,
        0x80, 0x00, 0x00, 0x80,
        0x5F, 0x9B, 0x34, 0xFB})
{}

static int from_hex(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
  if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
  return -1;
}

int UUID::compare(const UUID &u1, const UUID &u2) {
  uint8_t const *d1 = u1.data, *d2 = u2.data;
  for (unsigned int i=0; i < sizeof(UUID::data); ++i) {
    // byte order needs to be reversed
    if (d1[i] < d2[i]) return -1;
    if (d1[i] > d2[i]) return 1;
  }
  return 0;
}

UUID::UUID(uint16_t shortened) {
  for (unsigned int i=0; i < sizeof(data); ++i) data[i] = base.data[i];
    // byte order needs to be reversed
  data[2] = shortened >> 8;
  data[3] = shortened & 0x00ff;
}

UUID::UUID(const UUID &other) {
  for (unsigned int i=0; i < sizeof(data); ++i) data[i] = other.data[i];
}

UUID::UUID(const char *s) {
  for (unsigned int i=0; i < sizeof(data); ++i) data[i] = 0;
       
  uint8_t *p = data;
  int hi, lo;

  for (int i=0; i < 4; ++i) {
    hi = from_hex(*s++);
    lo = from_hex(*s++);
    if (hi < 0 || lo < 0) return;
    *p++ = (hi << 8) + lo;
  }

  if (*p++ != '-') return;

  for (int j=0; j < 3; ++j) {
    for (int i=0; i < 4; ++i) {
      hi = from_hex(*s++);
      lo = from_hex(*s++);
      if (hi < 0 || lo < 0) return;
      *p++ = (hi << 8) + lo;
    }

    if (*p++ != '-') return;
  }

  for (int i=0; i < 6; ++i) {
    hi = from_hex(*s++);
    lo = from_hex(*s++);
    if (hi < 0 || lo < 0) return;
    *p++ = (hi << 8) + lo;
  }
}

UUID UUID::base;
