#include "bluetooth.h"
#include "register_defs.h"

namespace HCI {
  #undef COMMAND
  #define COMMAND(ogf,ocf,name,send,expect) HCI::Command name = {OPCODE(ogf,ocf), send};
  #include "command_defs.h"
};

Baseband::Baseband(BufferedUART &uart, IOPin &shutdown) :
  uart(uart),
  shutdown(shutdown),
  reader(uart.rx, *this)
{
  reader.start();
  uart.set_delegate(this);
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

void Baseband::start() {
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
  reader.go();
}

void Baseband::event_packet(uint8_t code, size_t size) {
  extern IOPin led1;

  led1.set_value(1);
  uart.rx.advance(size);
}

void Baseband::acl_packet(uint16_t handle, uint8_t boundary, uint8_t broadcast, size_t size) {
}

void Baseband::synchronous_packet(uint16_t handle, uint8_t status, size_t size) {
}

StateMachine::StateMachine() : state(0) {
}

void StateMachine::start() {
  state = (State) &start;
}

UARTTransportReader::UARTTransportReader(RingBuffer &buf, Delegate &delegate) :
  delegate(delegate),
  input(buf)
{
}

void UARTTransportReader::start() {
  packet_size = 0;
  state = (State) &read_packet_indicator;
}

void UARTTransportReader::bad_packet_indicator() {
}

void UARTTransportReader::read_packet_indicator() {
  if (input.read_capacity() > 0) {
    uint8_t octet;
    input.read1(octet); // consume packet indicator

    switch (octet) {
    case HCI::EVENT_PACKET :
      go((State) &read_event_code_and_length);
      break;

    case HCI::COMMAND_PACKET :
    case HCI::ACL_PACKET :
    case HCI::SYNCHRONOUS_DATA_PACKET :
    default :
      go((State) &bad_packet_indicator);
    }
  }
}

void UARTTransportReader::read_event_code_and_length() {
  if (input.read_capacity() >= 2) {
    uint8_t octet;

    input.read1(event_code);
    input.read1(octet);
    packet_size = (size_t) octet;

    go((State) &read_event_parameters);
  }
}

void UARTTransportReader::read_event_parameters() {
  if (input.read_capacity() >= packet_size) {
    go((State) &event_packet_complete);
  }
}

void UARTTransportReader::event_packet_complete() {
  delegate.event_packet(event_code, packet_size);
  go((State) &read_packet_indicator);
}
