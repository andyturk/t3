#pragma once

#include "h4.h"

class Sequence {
 protected:
  H4Tranceiver &h4;

 public:
  Sequence(H4Tranceiver &h4) : h4(h4) {}
  virtual bool is_complete() const = 0;
  virtual bool command_complete(uint16_t opcode, Packet *p) = 0;
  virtual bool command_status(uint16_t opcode, Packet *p) = 0;
  virtual void restart() = 0;
  virtual void next() = 0;
};
