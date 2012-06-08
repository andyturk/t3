#include "bluetooth.h"
#include "register_defs.h"

namespace HCI {
  #undef COMMAND
  #define COMMAND(ogf,ocf,name,send,expect) HCI::Command name = {OPCODE(ogf,ocf), send};
  #include "command_defs.h"
};

void Baseband::configure() {
  shutdown.configure();
  uart.configure();
}

void Baseband::initialize() {
  shutdown.initialize();
  shutdown.set_value(0); // assert SHUTDOWN
  uart.initialize();
  uart.set_baud(115200);
  uart.set_enable(true);
  uart.set_interrupt_enable(true);

  shutdown.set_value(1); // clear SHUTDOWN
  CPU::delay(150); // wait 150 msec
}

void Baseband::send(const HCI::Command &cmd, ...) {
  va_list args;
  va_start(args, cmd);

  uart.tx.write1(HCI::COMMAND_PACKET);
  uart.tx.write1(cmd.opcode &0xff);
  uart.tx.write1(cmd.opcode >> 8);

  uint8_t &total_parameter_length = *uart.tx.write_ptr();
  uart.tx.write1(0); // total_parameter_length

  total_parameter_length = 0;

  const char *p = cmd.send;
  uint16_t u2;
  uint32_t u3, u4;

  while (*p) {
    switch (*p++) {
    case '1' :
      uart.tx.write1((uint8_t) va_arg(args, int));
      total_parameter_length += 1;
      break;
      
    case '2' :
      u2 = (uint16_t) va_arg(args, int);
      uart.tx.write1(u2 & 0xff);
      uart.tx.write1(u2 >> 8);
      total_parameter_length += 2;
      break;

    case '3' :
      u3 = va_arg(args, uint32_t);
      uart.tx.write1(u3 & 0xff);
      uart.tx.write1(u3 >> 8);
      uart.tx.write1(u3 >> 16);
      total_parameter_length += 3;
      break;

    case '4' :
      u4 = va_arg(args, uint32_t);
      uart.tx.write1(u4 & 0xff);
      uart.tx.write1(u4 >> 8);
      uart.tx.write1(u4 >> 16);
      uart.tx.write1(u4 >> 24);
      total_parameter_length += 4;
      break;

    case 'b' :
      uart.tx.write(va_arg(args, uint8_t *), 6);
      total_parameter_length += 6;
      break;

    case 'x' :
      uart.tx.write(va_arg(args, uint8_t *), 16);
      total_parameter_length += 16;
      break;

    case 'n' : {
      const uint8_t *p, *name = va_arg(args, const uint8_t *);
      for (p=name; *p; ++p);
      size_t len = p-name;
      uart.tx.write(name, len);
      while (len < 248) uart.tx.write1(0);
      total_parameter_length += 248;
      break;
    }
      
    case 'c' :
      uart.tx.write(va_arg(args, uint8_t *), 10);
      total_parameter_length += 10;
      break;

    case 'i' :
      uart.tx.write(va_arg(args, uint8_t *), 240);
      total_parameter_length += 240;
      break;

    case 'C' :
      uart.tx.write(va_arg(args, uint8_t *), 64);
      total_parameter_length += 64;
      break;

    case '[' :
    default :
      for(;;);
    }
  }

  uart.fill_tx_fifo();
  va_end(args);
}

void Baseband::error_occurred(BufferedUART *u) {
  for(;;);
}

void Baseband::data_received(BufferedUART *u) {
}


