#pragma once

#include <stdint.h>

#define COMMAND(ogf,ocf,name,send,expect) extern HCICommand name;
#define EVENT(code,name,args)
#define LE_EVENT(code,name,args)
#define OPCODE(ogf,ocf) ((ogf << 10) | ocf)

struct HCICommand {
  HCICommand(uint16_t opcode, const char *send) : opcode(opcode), send(send) {}

  const uint16_t opcode;
  const char *send;
};

struct BD_ADDR {
  uint8_t data[6];
};


#include "command_defs.h"
