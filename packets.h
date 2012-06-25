#pragma once

#include <stdarg.h>

#include "buffer.h"
#include "hci.h"

class Packet : public FlipBuffer<uint8_t> {
  enum {MAX_PACKET = 257};
  uint8_t buf[MAX_PACKET];

 public:
  Packet() : FlipBuffer<uint8_t>(buf, MAX_PACKET) {}
  void fput(HCI::Command const &cmd, ...);
  void fget(const char *format, ...);
};
