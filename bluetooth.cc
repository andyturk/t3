#include "bluetooth.h"
#include "register_defs.h"
#include "utils/uartstdio.h"

namespace HCI {
  #define BEGIN_COMMANDS
  #define COMMAND(ogf,ocf,name,send,expect) HCI::Command name = {OPCODE(ogf,ocf), send};
  #define END_COMMANDS

  #define BEGIN_EVENTS
  #define EVENT(code,name,args)
  #define END_EVENTS

  #define BEGIN_LE_EVENTS
  #define LE_EVENT(code,name,args)
  #define END_LE_EVENTS

  #include "command_defs.h"

  #undef BEGIN_COMMANDS
  #undef COMMAND
  #undef END_COMMANDS

  #undef BEGIN_EVENTS
  #undef EVENT
  #undef END_EVENTS

  #undef BEGIN_LE_EVENTS
  #undef LE_EVENT
  #undef END_LE_EVENTS
};

extern Baseband pan1323;

Baseband::Baseband(BufferedUART &uart, IOPin &shutdown) :
  uart(uart),
  shutdown(shutdown)
{
  uart.set_delegate(this);
}

void Baseband::initialize() {
  /*
  shutdown.initialize();
  shutdown.set_value(0); // assert SHUTDOWN
  uart.initialize();
  uart.set_baud(115200);
  uart.set_enable(true);
  uart.set_interrupt_enable(true);

  shutdown.set_value(1); // clear SHUTDOWN
  CPU::delay(150); // wait 150 msec
  */
}

void Baseband::receive(const char *format, ...) {
  va_list args;
  va_start(args, format);

  while (*format) {
    switch (*format++) {
    case '1' :
      uart.rx.read1(*va_arg(args, uint8_t *));
      break;

    case '2' :
      uart.rx.read(va_arg(args, uint8_t *), 2);
      break;

    case '3' : {
      uint32_t *arg = va_arg(args, uint32_t *);
      *arg = 0;
      uart.rx.read((uint8_t *) arg, 3);
      break;
    }

    case '4' :
      uart.rx.read(va_arg(args, uint8_t *), 4);
      break;

    case 'b' :
      uart.rx.read(va_arg(args, uint8_t *), 6);
      break;

    case 'x' :
      uart.rx.read(va_arg(args, uint8_t *), 16);
      break;

    case 'n' :
      uart.rx.read(va_arg(args, uint8_t *), 248);
      break;

    case 'c' :
      uart.rx.read(va_arg(args, uint8_t *), 10);
      break;

    case 'i' :
      uart.rx.read(va_arg(args, uint8_t *), 240);
      break;

    case 'C' :
      uart.rx.read(va_arg(args, uint8_t *), 64);
      break;

    case '[' :
    default :
      for(;;);
    }
  }

  va_end(args);
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
  reader.go(u->rx);
}

void Baseband::event_packet(UARTTransportReader &packet) {
  extern IOPin led1;

  led1.set_value(1);
  uart.rx.advance(packet.packet_size);
}

UARTTransportReader::UARTTransportReader() :
  StateMachine((State) &read_packet_indicator),
  delegate(0)
{
}

void UARTTransportReader::set_delegate(Delegate *d) {
  delegate = d;
}

void UARTTransportReader::bad_packet_indicator(RingBuffer &input) {
}

void UARTTransportReader::read_packet_indicator(RingBuffer &input) {
  if (input.read_capacity() > 0) {
    uint8_t octet;
    input.read1(octet); // consume packet indicator

    switch (octet) {
    case HCI::EVENT_PACKET :
      go((State) &read_event_code_and_length);
      go(input);
      break;

    case HCI::COMMAND_PACKET :
    case HCI::ACL_PACKET :
    case HCI::SYNCHRONOUS_DATA_PACKET :
    default :
      go((State) &bad_packet_indicator);
    }
  }
}

void UARTTransportReader::read_event_code_and_length(RingBuffer &input) {
  if (input.read_capacity() >= 2) {
    uint8_t octet;

    input.read1(event_code);
    input.read1(octet);
    packet_size = (size_t) octet;

    go((State) &read_event_parameters);
    go(input);
  }
}

void UARTTransportReader::read_event_parameters(RingBuffer &input) {
  if (input.read_capacity() >= packet_size) {
    if (delegate) {
      delegate->event_packet(*this);
    } else {
      input.advance(packet_size);
    }

    go((State) &read_packet_indicator);
    go(input);
  }
}

Pan1323Bootstrap::Pan1323Bootstrap(Baseband &b) : baseband(b) {
}

void Pan1323Bootstrap::event_packet(UARTTransportReader &packet) {
  if (packet.event_code == HCI::EVENT_COMMAND_COMPLETE) {
    uint8_t num_hci_packets, parameter;
    uint16_t opcode;

    pan1323.receive("121", &num_hci_packets, &opcode, &parameter);
    
    if (parameter == 0) {
      go(opcode);
      return;
    }
  }

  state = (State) &something_bad_happened;
}

void Pan1323Bootstrap::initialize() {
  pan1323.shutdown.set_value(0); // assert SHUTDOWN
  pan1323.uart.set_baud(115200);
  pan1323.uart.set_enable(true);
  pan1323.uart.set_interrupt_enable(true);
  pan1323.reader.set_delegate(this);
  state = (State) &reset_pending;

  pan1323.shutdown.set_value(1); // clear SHUTDOWN
  CPU::delay(150); // wait 150 msec
  pan1323.send(HCI::RESET);
}

void Pan1323Bootstrap::reset_pending(uint16_t opcode) {
  if (opcode == HCI::RESET.opcode) {
    state = (State) &read_version_info;
    
    pan1323.send(HCI::READ_LOCAL_VERSION_INFORMATION);
  } else {
    state = (State) &something_bad_happened;
  }
}

void Pan1323Bootstrap::read_version_info(uint16_t opcode) {
  if (opcode == HCI::READ_LOCAL_VERSION_INFORMATION.opcode) {
    pan1323.receive("12122", &pan1323.local_version_info.hci_version,
                              &pan1323.local_version_info.hci_revision,
                              &pan1323.local_version_info.lmp_version,
                              &pan1323.local_version_info.manufacturer_name,
                              &pan1323.local_version_info.lmp_subversion);

    state = (State) &baud_rate_negotiated;
    pan1323.send(HCI::PAN13XX_CHANGE_BAUD_RATE, 921600L);
  } else {
    state = (State) &something_bad_happened;
  }
}

void Pan1323Bootstrap::baud_rate_negotiated(uint16_t opcode) {
  if (opcode == HCI::PAN13XX_CHANGE_BAUD_RATE.opcode) {
    state = (State) &baud_rate_verified;
    pan1323.uart.set_baud(921600L);
    pan1323.send(HCI::READ_BD_ADDR);
  } else {
    state = (State) &something_bad_happened;
  }
}

void Pan1323Bootstrap::baud_rate_verified(uint16_t opcode) {
  extern IOPin led1;

  if (opcode == HCI::READ_BD_ADDR.opcode) {
    HCI::BD_ADDR addr;

    pan1323.receive("b", &addr);
    led1.set_value(1);
    UARTprintf("BD_ADDR is ");
    for (uint16_t i=0; i < sizeof(addr.data); ++i) UARTprintf("%02x:", addr.data[i]);
    UARTprintf("\n");
  } else {
    state = (State) &something_bad_happened;
  }
}

void Pan1323Bootstrap::something_bad_happened(uint16_t opcode) {
}

