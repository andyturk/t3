#pragma once

#include <cstddef>
#include <stdarg.h>

#include "hal.h"

namespace HCI {
  enum packet_indicators {
    COMMAND_PACKET          = 0x01,
    ACL_PACKET              = 0x02,
    SYNCHRONOUS_DATA_PACKET = 0x03,
    EVENT_PACKET            = 0x04
  };

#define COMMAND(ogf,ocf,name,send,expect) extern HCI::Command name;
#define EVENT(code,name,args)
#define LE_EVENT(code,name,args)

  struct Command {
    const uint16_t opcode;
    const char *send;
  };

  struct BD_ADDR {
    uint8_t data[6];
  };

  #include "command_defs.h"
};

class Baseband : public BufferedUART::Delegate {
 public:
  BufferedUART &uart;
  IOPin &shutdown;

  Baseband(BufferedUART &uart, IOPin &shutdown) : uart(uart), shutdown(shutdown) {}

  void configure();
  void initialize();

  void send(HCI::Command const &cmd, ...);
  void data_received(BufferedUART *u);
  void error_occurred(BufferedUART *u);
};
