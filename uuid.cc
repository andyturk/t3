#include <stddef.h>

#include "cc_stubs.h"
#include "uuid.h"

const uint8_t bluetooth_uuid[16] = {
  0xfb, 0x34, 0x9b, 0x5f,
  0x80, 0x00, 0x00, 0x80,
  0x00, 0x10, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

UUID::UUID() {
  memcpy(data, bluetooth_uuid, sizeof(bluetooth_uuid));
}

static int from_hex(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
  if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
  return -1;
}

int UUID::compare(const UUID &u1, const UUID &u2) {
  return memcmp(u1.data, u2.data, sizeof(UUID::data));
}

UUID::UUID(uint16_t shortened) {
  memcpy(data, bluetooth_uuid, sizeof(bluetooth_uuid));
  data[13] = (uint8_t) (shortened >> 8);
  data[14] = (uint8_t) (shortened & 0x00ff);
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

bool UUID::is_16bit() const {
  return !memcmp(data, base.data, 12) && !memcmp(data + 14, base.data + 14, 2);
}

UUID UUID::base;
