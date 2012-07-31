#pragma once

#include <stdint.h>
#include "h4.h"

class Script {
 protected:
  H4Tranceiver &h4;
  virtual void send(Packet *p);
  Packet *allocate_packet();

 public:
  Script(H4Tranceiver &t) : h4(t) {}

  virtual bool is_complete() const = 0;
  virtual bool is_pending() const = 0;
  virtual bool command_complete(uint16_t opcode, Packet *p) = 0;
  virtual bool command_status(uint16_t opcode, Packet *p) = 0;
  virtual void restart() = 0;
  virtual void next() = 0;
};

class CannedScript : public Script {
 protected:
  Packet bytes;
  uint16_t last_opcode;
  uint32_t baud_rate;
  void (*state)(CannedScript *);

 protected:
  Packet *next_canned_command();
  virtual void send(Packet *p);

 public:
  CannedScript(H4Tranceiver &t, const uint8_t *bytes, uint16_t length);

  virtual bool is_complete() const;
  virtual bool is_pending() const;
  virtual bool command_complete(uint16_t opcode, Packet *p);
  virtual bool command_status(uint16_t opcode, Packet *p);
  virtual void restart();
  virtual void next() { (*state)(this); }
};
