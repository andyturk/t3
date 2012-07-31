#pragma once

#include <stdint.h>
#include "h4.h"

class Sequence {
 protected:
  H4Tranceiver &h4;

 public:
  Sequence(H4Tranceiver &t) : h4(t) {}

  virtual bool is_complete() const = 0;
  virtual bool is_pending() const = 0;
  virtual bool command_complete(uint16_t opcode, Packet *p) = 0;
  virtual bool command_status(uint16_t opcode, Packet *p) = 0;
  virtual void restart() = 0;
  virtual void next() = 0;
};

class CannedSequence : public Sequence {
 protected:
  Packet bytes;
  Packet command;
  uint16_t last_opcode;
  uint32_t baud_rate;

 public:
  CannedSequence(H4Tranceiver &t, const uint8_t *bytes, uint16_t length);

  virtual bool is_complete() const;
  virtual bool is_pending() const;
  virtual bool command_complete(uint16_t opcode, Packet *p);
  virtual bool command_status(uint16_t opcode, Packet *p);
  virtual void restart();
  virtual void next();
};
