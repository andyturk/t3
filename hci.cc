#include "hci.h"

#undef COMMAND
#define COMMAND(ogf,ocf,name,send,expect) HCICommand name(OPCODE(ogf,ocf),send);

#include "command_defs.h"
