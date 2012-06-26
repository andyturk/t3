#include "hci.h"
#include "utils/uartstdio.h"

#define BEGIN_COMMANDS
#define COMMAND(ogf,ocf,name,send,expect) HCI::Command name = {OPCODE(ogf,ocf), send};
#define END_COMMANDS

#define BEGIN_EVENTS
#define EVENT(code,name,args)
#define END_EVENTS

#define BEGIN_LE_EVENTS
#define LE_EVENT(code,name,args)
#define END_LE_EVENTS

namespace HCI {
  #include "command_defs.h"
};

#undef BEGIN_COMMANDS
#undef COMMAND
#undef END_COMMANDS
#undef BEGIN_EVENTS
#undef EVENT
#undef END_EVENTS
#undef BEGIN_LE_EVENTS
#undef LE_EVENT
#undef END_LE_EVENTS

using namespace HCI;

void HCI::Packet::fput(HCI::Command const &cmd, ...) {
  va_list args;
  va_start(args, cmd);

  reset();

  put(HCI::COMMAND_PACKET);
  put(cmd.opcode & 0xff);
  put(cmd.opcode >> 8);

  set_mark();

  put(0x00); // will be replaced with length later

  const size_t param0 = get_position();

  const char *p = cmd.send;
  uint8_t u1;
  uint16_t u2;
  uint32_t u4;

  while (*p) {
    switch (*p++) {
    case '1' :
      u1 = va_arg(args, int);
      put(u1);
      break;
      
    case '2' :
      u2 = va_arg(args, int);
      put(u2 & 0xff);
      u2 >>= 8;
      put(u2 & 0xff);
      break;

    case '3' :
      u4 = va_arg(args, uint32_t);
      put(u4 & 0xff);
      u4 >>= 8;
      put(u4 & 0xff);
      u4 >>= 8;
      put(u4 & 0xff);
      break;

    case '4' :
      u4 = va_arg(args, uint32_t);
      put(u4 & 0xff);
      u4 >>= 8;
      put(u4 & 0xff);
      u4 >>= 8;
      put(u4 & 0xff);
      u4 >>= 8;
      put(u4 & 0xff);
      break;

    case 'b' : {
      uint8_t *p = va_arg(args, uint8_t *);
      for (int i=0; i < 6; ++i) put(*p++);
      break;
    }

    case 'X' : {
      uint8_t *p = va_arg(args, uint8_t *);
      for (int i=0; i < 16; ++i) put(*p++);
      break;
    }

    case 'n' : {
      const uint8_t *p, *name = va_arg(args, const uint8_t *);
      size_t start = get_position();
      for (p=name; *p; ++p) put(*p);
      for(size_t len = get_position() - start; len < 248; ++len) put(0x00);
      break;
    }
      
    case 'c' : {
      uint8_t *p = va_arg(args, uint8_t *);
      for (int i=0; i < 10; ++i) put(*p++);
      break;
    }

    case 'i' : {
      uint8_t *p = va_arg(args, uint8_t *);
      for (int i=0; i < 240; ++i) put(*p++);
      break;
    }

    case 'S' : {
      uint8_t *p = va_arg(args, uint8_t *);
      for (int i=0; i < 64; ++i) put(*p++);
      break;
    }

    case ' ' : // skip spaces
      break;

    case '[' :
    default :
      for(;;);
    }
  }

  size_t total_parameter_length = get_position() - param0;
  assert(total_parameter_length <= 255);

  flip();
  back_to_mark();
  put(total_parameter_length);
  rewind();

  va_end(args);
}

void HCI::Packet::fget(const char *format, ...) {
  va_list args;
  va_start(args, format);
  uint8_t *p, u1;

  while (*format) {
    switch (*format++) {
    case 'C' :
      u1 = get();
      assert(u1 == COMMAND_PACKET);
      break;

    case 'A' :
      u1 = get();
      assert(u1 == ACL_PACKET);
      break;

    case 'D' :
      u1 = get();
      assert(u1 == SYNCHRONOUS_DATA_PACKET);
      break;

    case 'E' :
      u1 = get();
      assert(u1 == EVENT_PACKET);
      break;

    case '0' : {
      uint8_t actual = get();
      assert(actual == 0);
      break;
    }
      
    case '1' :
      p = va_arg(args, uint8_t *);
      p[0] = get();
      break;

    case '2' :
      p = va_arg(args, uint8_t *);
      p[0] = get();
      p[1] = get();
      break;

    case '3' : {
      uint32_t *arg = va_arg(args, uint32_t *);
      *arg = 0;
      p = (uint8_t *) arg;

      p[0] = get();
      p[1] = get();
      p[2] = get();
      break;
    }

    case '4' :
      p = va_arg(args, uint8_t *);
      p[0] = get();
      p[1] = get();
      p[2] = get();
      p[3] = get();
      break;

    case 'b' :
      p = va_arg(args, uint8_t *);
      for (int i=0; i < 6; ++i) p[i] = get();
      break;

    case 'X' :
      p = va_arg(args, uint8_t *);
      for (int i=0; i < 16; ++i) p[i] = get();
      break;

    case 'n' :
      p = va_arg(args, uint8_t *);
      for (int i=0; i < 248; ++i) p[i] = get();
      break;

    case 'c' :
      p = va_arg(args, uint8_t *);
      for (int i=0; i < 10; ++i) p[i] = get();
      break;

    case 'i' :
      p = va_arg(args, uint8_t *);
      for (int i=0; i < 240; ++i) p[i] = get();
      break;

    case 'S' :
      p = va_arg(args, uint8_t *);
      for (int i=0; i < 64; ++i) p[i] = get();
      break;

    case '%' : {
      switch (*format++) {
      case '1' : {
        uint8_t expected = (uint8_t) va_arg(args, int);
        uint8_t actual = get();
        assert(actual == expected);
        break;
      }
        
      case '2' : {
        uint16_t expected = (uint16_t) va_arg(args, int);
        uint16_t actual = get() + (get() << 8);
        assert(actual == expected);
        break;
      }

      case '3' : {
        uint32_t expected = va_arg(args, uint32_t);
        uint32_t actual = get() + (get() << 8) + (get() << 16);
        assert(actual == expected);
        break;
      }

      case '4' : {
        uint32_t expected = va_arg(args, uint32_t);
        uint32_t actual = get() + (get() << 8) + (get() << 16) + (get() << 24);
        assert(actual == expected);
        break;
      }

      default :
        assert(false);
        break;
      }
      break;
    }
      
    case '?' : // any byte
      get();
      break;

    case ' ' : // skip spaces
      break;

    case '[' :
    default :
      for(;;);
    }
  }

  va_end(args);
}

BBand::BBand(UART &u, IOPin &s) :
  uart(u),
  shutdown(s),
  rx(0), rx_state(0),
  tx(0), 
  free_packets(0),
  incoming_packets(0),
  packet_handler(&deallocate_packet)
{
  // initialize free packet pool
  // all packets are free to start with
  free_packets = &packet_pool[0];
  for (int i=0; i < PACKET_POOL_SIZE-1; ++i) packet_pool[i].next = &packet_pool[i+1];
}

void BBand::drain_uart() {
  assert(rx == 0 || rx->get_remaining() > 0);

  while (rx && (rx->get_remaining() > 0) && uart.can_read()) {
    uint8_t byte;
    uart.read(&byte, 1);
    rx->put(byte);
    if (rx->get_remaining() == 0) rx_state(this);
  }
}

void BBand::fill_uart() {
  while (tx && (tx->get_remaining() > 0) && uart.can_write()) {
    uint8_t byte = tx->get();
    uart.write(&byte, 1);
    if (tx->get_remaining() == 0) {
      uart.set_interrupt_enable(false);
      tx->next = free_packets;
      free_packets = tx;
      tx = 0;
      uart.set_interrupt_enable(true);
    }
  }

  if (tx->get_remaining() > 0) {
    uart.set_interrupt_sources(UART::RX | UART::TX | UART::ERROR);
  }
}

void BBand::uart_interrupt_handler() {
  uint32_t cause = uart.clear_interrupt_cause(UART::RX | UART::TX | UART::ERROR);

  assert(!(cause & UART::ERROR));

  if (cause & UART::TX) fill_uart();
  if (cause & UART::RX) drain_uart();

  cause = UART::RX | UART::ERROR;
  // enable the tx interrupt if there's more data in the buffer
  if (tx && tx->get_remaining() > 0) cause |= UART::TX;
  uart.set_interrupt_sources(cause);
}

void BBand::initialize() {
  shutdown.set_value(0); // assert SHUTDOWN
  uart.set_enable(false);
  uart.set_fifo_enable(false);
  uart.set_fifo_enable(true);
  uart.set_baud(115200);
  uart.set_enable(true);

  tx = allocate_packet();
  tx->reset();
  tx->fput(HCI::RESET);

  rx = allocate_packet();
  rx->reset();
  rx->set_limit(1);
  rx_state = &rx_expect_packet_indicator;

  uart.set_interrupt_sources(UART::RX | UART::ERROR);
  packet_handler = &reset_pending;
  command_complete_handler = &initialization_command_complete;

  shutdown.set_value(1); // clear SHUTDOWN
  uart.set_interrupt_enable(true);

  CPU::delay(150); // wait 150 msec
  fill_uart();
}

void BBand::rx_expect_packet_indicator() {
  switch (rx->peek(-1)) {
  case HCI::EVENT_PACKET :
    rx->set_limit(1+1+1); // indicator, event code, param length
    rx_state = &rx_expect_event_code_and_length;
    break;

  case HCI::COMMAND_PACKET :
  case HCI::ACL_PACKET :
  case HCI::SYNCHRONOUS_DATA_PACKET :
  default :
    assert(false);
  }
}

void BBand::rx_expect_event_code_and_length() {
  uint8_t param_length = rx->peek(-1);

  if (param_length > 0) {
    rx->set_limit(1+1+1 + param_length);
    rx_state = &rx_expect_event_parameters;
  } else {
    // there are no params, so just chain to the next state
    rx_expect_event_parameters();
  }
}

void BBand::rx_expect_event_parameters() {
  // assert that we're handling the uart interrupt so we won't
  // be interrupted

  rx->flip();

  HCI::Packet *&tail = incoming_packets;
  while (tail != 0) tail = tail->next;
  assert(tail == 0);

  tail = rx;
  rx->next = 0;
  
  rx = allocate_packet();
  rx->reset();
  rx->set_limit(1);
  rx_state = &rx_expect_packet_indicator;
}

HCI::Packet *BBand::allocate_packet() {
  uart.set_interrupt_enable(false);
  assert(free_packets != 0);

  HCI::Packet *value = free_packets;
  free_packets = value->next;
  value->next = 0;

  uart.set_interrupt_enable(true);
  return value;
}

void BBand::deallocate_packet(HCI::Packet *p) {
  uart.set_interrupt_enable(false);
  p->next = free_packets;
  free_packets = p;
  uart.set_interrupt_enable(true);
}

void BBand::process_incoming_packets() {
  HCI::Packet *p = 0;

  do {
    uart.set_interrupt_enable(false);
    p = incoming_packets;
    if (p) {
      incoming_packets = p->next;
      p->next = 0;
    }
    uart.set_interrupt_enable(true);

    if (p) standard_packet_handler(p);
  } while (p);
}

void BBand::send(Packet *p) {
  assert(tx == 0);
  p->next = 0;
  tx = p;
  fill_uart();
}

void BBand::standard_packet_handler(Packet *p) {
  switch (p->get()) {
  case EVENT_PACKET : {
    uint8_t event = p->get();
    uint8_t ignored_param_length __attribute__ ((unused)) = p->get();

    if (event == EVENT_COMMAND_COMPLETE) {
      uint16_t opcode;

      p->fget("12", &command_packet_budget, &opcode);
      command_complete_handler(this, opcode, p);
    } else {
      event_handler(this, event, p);
    }
    break;
  }
    
  case COMMAND_PACKET :
  case ACL_PACKET :
  case SYNCHRONOUS_DATA_PACKET :
  default :
    assert(false);
  }
}

void BBand::initialization_command_complete(uint16_t opcode, Packet *p) {
  uint8_t status = p->get();
  assert(status == SUCCESS);

  switch (opcode) {
  case OPCODE_RESET :
    p->reset();
    p->fput(READ_LOCAL_VERSION_INFORMATION);
    break;

  case OPCODE_READ_LOCAL_VERSION_INFORMATION : {
    uint8_t hci_version, lmp_version;
    uint16_t hci_revision, manufacturer_name, lmp_subversion;
    p->fget("12122", &hci_version, &hci_revision, &lmp_version, &manufacturer_name, &lmp_subversion);
    assert(hci_version == SPECIFICATION_4_0);

    p->reset();
    p->fput(PAN13XX_CHANGE_BAUD_RATE, 921600L);
    break;
  }

  case OPCODE_PAN13XX_CHANGE_BAUD_RATE :
    uart.set_baud(921600L);
    p->reset();
    p->fput(READ_BD_ADDR);
    break;

  case OPCODE_READ_BD_ADDR :
    p->fget("b", &bd_addr);
    p->reset();

    // initialize the patch state
    extern const unsigned char PatchXETU[];
    extern const int PatchXETULength;
  
    patch_state.expected_opcode = 0;
    patch_state.offset = 0;
    patch_state.length = PatchXETULength;
    patch_state.data = (uint8_t *) PatchXETU;

    command_complete_handler = &patch_command_complete;
    patch_command_complete(0, p);
    p = 0;
  }

  if (p) send(p);
}

void BBand::patch_command_complete(uint16_t opcode, Packet *p) {
  assert(opcode == patch_state.expected_opcode);

  if (patch_state.offset < patch_state.length) {
    assert((patch_state.length - patch_state.offset) >= 4);

    p->reset();
    uint8_t *cmd = patch_state.data + patch_state.offset;
    size_t command_length = 4 + cmd[3];
    patch_state.expected_opcode = (cmd[2] << 8) + cmd[1];

    for (size_t i=0; i < command_length; ++i) p->put(cmd[i]);
    patch_state.offset += command_length;
    UARTprintf("patch command %04x @ %d (%d)\n",
               patch_state.expected_opcode, patch_state.offset - command_length, command_length);
    p->flip();
    send(p);
  } else {
    deallocate_packet(p);
    UARTprintf("initialization complete\n");
  }
}

void BBand::reset_pending(HCI::Packet *p) {
  p->fget("E %1 ?? %2 0", EVENT_COMMAND_COMPLETE, RESET.opcode);
  // use the same packet to send the next message
  p->reset();
  p->fput(READ_LOCAL_VERSION_INFORMATION);
  packet_handler = &read_version_info;
  send(p);
}

void BBand::read_version_info(HCI::Packet *p) {
  uint8_t hci_version, lmp_version;
  uint16_t hci_revision, manufacturer_name, lmp_subversion;

  p->fget("E %1 ?? %2 0 12122", EVENT_COMMAND_COMPLETE, READ_LOCAL_VERSION_INFORMATION.opcode,
          &hci_version, &hci_revision, &lmp_version,
          &manufacturer_name, &lmp_subversion);

  assert(hci_version == SPECIFICATION_4_0);

  p->reset();
  p->fput(PAN13XX_CHANGE_BAUD_RATE, 921600L);
  packet_handler = &baud_rate_negotiated;
  send(p);
}

void BBand::baud_rate_negotiated(Packet *p) {
  p->fget("E %1 ?? %2 0", EVENT_COMMAND_COMPLETE, PAN13XX_CHANGE_BAUD_RATE.opcode);

  uart.set_baud(921600L);
  p->reset();
  p->fput(READ_BD_ADDR);
  packet_handler = &read_bd_addr;
  send(p);
}

void BBand::read_bd_addr(Packet *p) {
  p->fget("E %1 ?? %2 0 b", EVENT_COMMAND_COMPLETE, READ_BD_ADDR.opcode, &bd_addr);
  p->reset();

  UARTprintf("BD_ADDR is ");
  for (uint16_t i=0; i < sizeof(bd_addr.data); ++i) UARTprintf("%02x:", bd_addr.data[i]);
  UARTprintf("\n");

  packet_handler = &send_patch_command;

  // initialize the patch state
  extern const unsigned char PatchXETU[];
  extern const int PatchXETULength;
  
  patch_state.expected_opcode = 0;
  patch_state.offset = 0;
  patch_state.length = PatchXETULength;
  patch_state.data = (uint8_t *) PatchXETU;

  send_patch_command(p);
}

void BBand::send_patch_command(Packet *p) {
  if (patch_state.expected_opcode != 0) {
    p->fget("E %1 ?? %2 0", EVENT_COMMAND_COMPLETE, patch_state.expected_opcode);
  }

  if (patch_state.offset < patch_state.length) {
    assert((patch_state.length - patch_state.offset) >= 4);

    p->reset();
    uint8_t *cmd = patch_state.data + patch_state.offset;
    size_t command_length = 4 + cmd[3];
    patch_state.expected_opcode = (cmd[2] << 8) + cmd[1];

    for (size_t i=0; i < command_length; ++i) p->put(cmd[i]);
    patch_state.offset += command_length;
    UARTprintf("patch command %04x @ %d (%d)\n",
               patch_state.expected_opcode, patch_state.offset - command_length, command_length);
    p->flip();
    send(p);
  } else {
    deallocate_packet(p);
    UARTprintf("initialization complete\n");
  }
}
