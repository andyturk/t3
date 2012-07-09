#include "hci.h"
#include "att.h"
#include "utils/uartstdio.h"

using namespace HCI;

// 1 - integer (1)
// 2 - integer (2)
// 3 - integer (3)
// 4 - integer (4)
// 8 - integer (8)
// b - BD_ADDR (6)
// X - data (16)
// * - unknown number of bytes
// [ - begin array (0)
// ] - end array (0)
// n - null padded UTF-8 string (248)
// c - channel classification (10)
// i - extended inquiry (240)
// S - supported commands (64)

const char hex_digits[16] = {
  '0', '1', '2', '3', '4', '5', '6', '7',
  '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

/*
void Packet::command(uint16_t opcode, const char *format, ...) {
  reset();
  put(HCI::COMMAND_PACKET);
  
  put(opcode & 0xff);
  put(opcode >> 8);

  set_mark();

  put(0x00); // will be replaced with length later

  if (format == 0) {
    flip();
  } else {
    size_t param0 = get_position();

    va_list args;
    va_start(args, format);

    vfput(format, args);

    size_t total_parameter_length = get_position() - param0;
    assert(total_parameter_length <= 255);

    flip();
    back_to_mark();
    put(total_parameter_length);
    rewind();

    va_end(args);
  }
}

void Packet::vfput(const char *format, va_list args) {
  uint8_t u1;
  uint16_t u2;
  uint32_t u4;

  while (*format) {
    switch (*format++) {
    case 'C' :
      put(COMMAND_PACKET);
      break;

    case 'A' :
      put(ACL_PACKET);
      break;

    case 'D' :
      put(SYNCHRONOUS_DATA_PACKET);
      break;

    case 'E' :
      put(EVENT_PACKET);
      break;

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

    case '*' : {
      uint8_t *p = va_arg(args, uint8_t *);
      uint32_t len = va_arg(args, uint32_t);

      for (unsigned int i=0; i < len; ++i) put(*p++);
      break;
    }

    case ' ' : // skip spaces
      break;

    case '[' :
    default :
      for(;;);
    }
  }
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

    case '8' :
      p = va_arg(args, uint8_t *);
      for (int i=0; i < 8; ++i) *p++ = get();
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
*/

void Packet::dump() {
  for (unsigned int i=position; i < limit; ++i) {
    char c1 = hex_digits[storage[i]] >> 4;
    char c2 = hex_digits[storage[i]] &0x0f;
    UARTprintf("%c%c ", c1, c2);
  }
}

BBand::BBand(UART &u, IOPin &s) :
  uart(u),
  shutdown(s),
  rx(0), rx_state(0),
  tx(0), 
  indicator_packet(indicator_packet_storage, sizeof(indicator_packet_storage)),
  event_handler(&default_event_handler)
{
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
      command_packet_pool.deallocate((CommandPacket *) tx);
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

  rx_new_packet();

  uart.set_interrupt_sources(UART::RX | UART::ERROR);

  tx = command_packet_pool.allocate();
  assert(tx != 0);

  command_complete_handler = &cold_boot;

  cold_boot(0, tx);

  shutdown.set_value(1); // clear SHUTDOWN
  uart.set_interrupt_enable(true);

  CPU::delay(150); // wait 150 msec
  fill_uart();
}

void BBand::rx_new_packet() {
  indicator_packet.reset();
  indicator_packet.set_limit(1);
  rx = &indicator_packet;
  rx_state = &rx_packet_indicator;
}

void BBand::rx_packet_indicator() {
  uint8_t ind = indicator_packet.peek(-1);

  switch (ind) {
  case HCI::EVENT_PACKET :
    rx = command_packet_pool.allocate();
    assert(rx != 0);

    rx->set_limit(1+1+1); // indicator, event code, param length
    rx->put(ind);
    rx_state = &rx_event_header;
    break;

  case HCI::ACL_PACKET :
    rx = acl_packet_pool.allocate();
    assert(rx != 0);

    rx->set_limit(1+4);
    rx->put(ind);
    rx_state = &rx_acl_header;
    break;

  case HCI::COMMAND_PACKET :
  case HCI::SYNCHRONOUS_DATA_PACKET :
  default :
    assert(false);
  }
}

void BBand::rx_acl_header() {
  uint16_t length = (rx->peek(-1) << 8) + (rx->peek(-2));
  rx->set_limit(1+4+length);
  rx_state = &rx_queue_received_packet;
}

void BBand::rx_event_header() {
  uint8_t param_length = rx->peek(-1);

  if (param_length > 0) {
    rx->set_limit(1+1+1 + param_length);
    rx_state = &rx_queue_received_packet;
  } else {
    // there are no params, so just chain to the next state
    rx_queue_received_packet();
  }
}

void BBand::rx_queue_received_packet() {
  // assert that we're handling the uart interrupt so we won't
  // be interrupted

  if (uart.can_read()) {
    UARTprintf("warning: UART fifo not empty after received packet\n");
  }

  rx->flip();
  rx->join(&incoming_packets);
  rx_new_packet();
}

void BBand::deallocate_packet(HCI::Packet *p) {
  __asm ("cpsid i");

  switch (p->get(0)) {
  case COMMAND_PACKET :
  case EVENT_PACKET :
    command_packet_pool.deallocate((HCI::CommandPacket *)p);
    break;
  default :
    assert(false);
  }

  __asm ("cpsie i");
}

void BBand::process_incoming_packets() {
  HCI::Packet *p;

  do {
    uart.set_interrupt_enable(false);

    Ring<Packet>::Iterator i = incoming_packets.rbegin();
    if (i != incoming_packets.end()) {
      p = i;
      p->join(p);
    } else {
      p = 0;
    }
    uart.set_interrupt_enable(true);

    if (p) standard_packet_handler(p);
  } while (p);
}

void BBand::send(Packet *p) {
  assert(tx == 0);
  p->prepare_for_tx();
  tx = p;
  fill_uart();
}

void BBand::default_event_handler(uint8_t event, Packet *p) {
  switch (event) {
  case EVENT_DISCONNECTION_COMPLETE : {
    uint16_t handle;
    uint8_t status, reason;

    *p >> status >> handle >> reason;
    handle &= 0x0fff;

    UARTprintf("disconnected 0x%04x (with status: 0x%02x) because 0x%02x\n", handle, status, reason);
    break;
  }
    
  case EVENT_LE_META_EVENT : {
    uint8_t subevent;
    *p >> subevent;
    le_event_handler(subevent, p);
    break;
  }

  default :
    UARTprintf("discarding event 0x%02x\n", event);
  }

  deallocate_packet(p);
}

void BBand::le_event_handler(uint8_t subevent, Packet *p) {
  switch(subevent) {
  case LE_EVENT_CONNECTION_COMPLETE : {
    HCI::BD_ADDR peer_address;
    uint8_t status, role, peer_address_type, master_clock_accuracy;
    uint16_t connection_handle, conn_interval, conn_latency, supervision_timeout;

    *p >> status >> connection_handle >> role;
    *p >> peer_address_type >> peer_address;
    *p >> conn_interval >> conn_latency;
    *p >> supervision_timeout >> master_clock_accuracy;

    char buf[HCI::BD_ADDR::PP_BUF_LEN];
    const char *type;

    switch (peer_address_type) {
    case 0 :
      type = "public";
      break;

    case 1 :
      type = "random";
      break;

    default :
      type = "unknown";
      break;
    }

    const char *role_name;
    
    switch (role) {
    case 0 :
      role_name = "master";
      break;

    case 1 :
      role_name = "slave";
      break;

    default :
      role_name = "unknown";
      break;
    }

    peer_address.pretty_print(buf);

    UARTprintf("connection to %s completed with status %d\n", buf, status);
    UARTprintf("  handle = 0x%04x, addr_type = %s\n", connection_handle, type);
    UARTprintf("  role = %s, interval = %d, latency = %d, timeout = %d\n", role_name, conn_interval, conn_latency, supervision_timeout);
    UARTprintf("  clock_accuracy = %d\n", master_clock_accuracy);
    break;
  }
  case LE_EVENT_ADVERTISING_REPORT :
  case LE_EVENT_CONNECTION_UPDATE_COMPLETE :
  case LE_EVENT_READ_REMOTE_USED_FEATURES_COMPLETE :
  case LE_EVENT_LONG_TERM_KEY_REQUEST :
  default :
    UARTprintf("ignoring unrecognized LE event: 0x%02x\n", subevent);
    break;
  }
}

void BBand::standard_packet_handler(Packet *p) {
  uint8_t packet_indicator = p->get();
  switch (packet_indicator) {
  case EVENT_PACKET : {
    uint8_t event = p->get();
    uint8_t ignored_param_length __attribute__ ((unused)) = p->get();

    if (event == EVENT_COMMAND_COMPLETE) {
      uint16_t opcode;

      *p >> command_packet_budget >> opcode;
      command_complete_handler(this, opcode, p);
    } else {
      event_handler(this, event, p);
    }
    break;
  }
    
  case ACL_PACKET : {
    uint16_t raw_handle, length;
    *p >> raw_handle >> length;

    uint16_t handle = raw_handle & 0x0fff;
    uint8_t bc = (raw_handle >> 14) & 0x0003;
    uint8_t pb = (raw_handle >> 12) & 0x0003;

    acl_packet_handler(handle, pb, bc, p);
    break;
  }

  case COMMAND_PACKET :
  case SYNCHRONOUS_DATA_PACKET :
  default :
    UARTprintf("discarding unknown packet of type %d\n", packet_indicator);
    deallocate_packet(p);
  }
}

void BBand::acl_packet_handler(uint16_t handle, uint8_t pb, uint8_t bc, Packet *p) {
  UARTprintf("ACL data, handle = 0x%04x, pb = %d, bc = %d\n", handle, pb, bc);

  l2cap_packet_handler(p);
  acl_packet_pool.deallocate((HCI::ACLPacket *) p);
}

void BBand::l2cap_packet_handler(Packet *p) {
  uint16_t cid, length;
  *p >> length >> cid;

  switch(cid) {
  case L2CAP::ATTRIBUTE_CID :
    att_packet_handler(p);
    break;

  default :
    UARTprintf("unrecognized L2CAP CID: 0x%04x\n", cid);
    break;
  }
}

void BBand::att_packet_handler(Packet *p) {
  uint8_t opcode;

  *p >> opcode;

  switch (opcode) {
  case ATT::OPCODE_ERROR : {
    uint16_t handle;
    uint8_t error;

    *p >> opcode >> handle >> error;
    UARTprintf("ATT error for opcode: 0x%02x, handle: 0x%04x, error: 0x%02x\n", opcode, handle, error);
    break;
  }

  case ATT::OPCODE_FIND_TYPE_BY_VALUE_REQUEST : {
    uint16_t first_handle, last_handle, type;
    *p >> first_handle >> last_handle >> type;
    break;
  }

  default :
    UARTprintf("unrecognized att opcode: 0x%02x\n", opcode);
    break;
  }
}

void BBand::cold_boot(uint16_t opcode, Packet *p) {
  if (opcode != 0) {
    uint8_t status = p->get();
    assert(status == SUCCESS);
  }

  switch (opcode) {
  case 0 :
    // This case runs before the uart is enabled, so we simply
    // format the tx packet and set p=0 to prevent it from
    // being sent at this time. Later on, the calling code will
    // send the contents of the tx buffer through the uart.
    *tx << OPCODE_RESET;
    tx->prepare_for_tx();
    p = 0;

    break;

  case OPCODE_RESET :
    *p << OPCODE_READ_LOCAL_VERSION_INFORMATION;
    break;

  case OPCODE_READ_LOCAL_VERSION_INFORMATION : {
    uint8_t hci_version, lmp_version;
    uint16_t hci_revision, manufacturer_name, lmp_subversion;

    *p >> hci_version >> hci_revision >> lmp_version >> manufacturer_name >> lmp_subversion;
    assert(hci_version == SPECIFICATION_4_0);

    *p << OPCODE_PAN13XX_CHANGE_BAUD_RATE << (uint32_t) 921600L;
    break;
  }

  case OPCODE_PAN13XX_CHANGE_BAUD_RATE :
    uart.set_baud(921600L);

    // initialize the patch state
    extern const uint8_t cc256x_init_script[];
    extern const uint32_t cc256x_init_script_size;
  
    patch_state.expected_opcode = 0;
    patch_state.offset = 0;
    patch_state.length = cc256x_init_script_size;
    patch_state.data = cc256x_init_script;

    command_complete_handler = &upload_patch;
    upload_patch(0, p);
    p = 0;
    break;

  default :
    UARTprintf("discarding unrecognized event packet (%02x) in during cold boot\n", opcode);
    deallocate_packet(p);
    p = 0;
  }

  if (p) send(p);
}

void BBand::upload_patch(uint16_t opcode, Packet *p) {
  assert(!uart.can_read());
  assert(opcode == patch_state.expected_opcode);

  if (patch_state.offset < patch_state.length) {
    assert((patch_state.length - patch_state.offset) >= 4);

    p->reset();
    const uint8_t *cmd = patch_state.data + patch_state.offset;
    size_t command_length = 4 + cmd[3];
    patch_state.expected_opcode = (cmd[2] << 8) + cmd[1];

    for (size_t i=0; i < command_length; ++i) p->put(cmd[i]);
    patch_state.offset += command_length;
    //UARTprintf("patch command %04x @ %d (%d)\n",
    //           patch_state.expected_opcode, patch_state.offset - command_length, command_length);
    p->flip();
    send(p);
  } else {
    UARTprintf("patching complete\n");
    command_complete_handler = &warm_boot;
    warm_boot(0, p);
  }
}

/*
const uint8_t warm_boot_patch[] = {
  0x01, I2(OPCODE_READ_BD_ADDR), 0,
  0x01, I2(OPCODE_READ_BUFFER_SIZE_COMMAND), 0,
  0x01, I2(OPCODE_WRITE_PAGE_TIMEOUT), 0, I2(0x2000),
  0x01, I2(OPCODE_READ_PAGE_TIMEOUT), 0,
  // WRITE_LOCAL_NAME_COMMAND
  0x01, I2(OPCODE_WRITE_SCAN_ENABLE), 1, 0x03,
  0x01, I2(OPCODE_WRITE_CLASS_OF_DEVICE), 4, I4(0x0098051c),
  0x01, I2(OPCODE_SET_EVENT_MASK), 8, I4(0xffffffff), I4(0x20001fff),
  0x01, I2(OPCODE_WRITE_LE_HOST_SUPPORT), 2, 1, 1,
  0x01, I2(OPCODE_LE_SET_EVENT_MASK), 8, I4(0xffffffff), I4(0xffffffff),
  0x01, I2(OPCODE_LE_READ_BUFFER_SIZE), 0,
  0x01, I2(OPCODE_LE_READ_SUPPORTED_STATES), 0,
  // LE_SET_ADVERTISING_PARAMETERS
  0x01, I2(OPCODE_LE_SET_ADVERTISING_DATA), 8, 7, 0x02, 0x01, 0x05, 0x03, 0x02, 0xf0, 0xff,
  0x01, I2(OPCODE_LE_SET_SCAN_RESPONSE_DATA), 8, 7, 0x02, 0x01, 0x05, 0x03, 0x02, 0xf0, 0xff,
  0x01, I2(OPCODE_LE_SET_ADVERTISE_ENABLE), 1, 0x01,
};
*/

void BBand::warm_boot(uint16_t opcode, Packet *p) {
  assert(!uart.can_read());
  if (opcode != 0) {
    uint8_t status = p->get();

    if (status != SUCCESS) {
      uint16_t ocf = opcode & 0x03ff;
      uint16_t ogf = opcode >> 10;

      UARTprintf("bad command status (0x%x): ogf: %d ocf: 0x%04x\n", status, ogf, ocf);
      for (;;);
    }
  }

  switch (opcode) {
  case 0 : {
    UARTprintf("warm boot...\n");
    uint8_t deep_sleep_enable = 0x00;
    *p << OPCODE_SLEEP_MODE_CONFIGURATIONS << (uint8_t) 0x01 << deep_sleep_enable;
    *p << (uint8_t) 0 << (uint32_t) 0xffffffff << (uint8_t) 100 << (uint8_t) 0;
    //p->command(OPCODE_SLEEP_MODE_CONFIGURATIONS, "111411", 0x01, deep_sleep_enable, 0, 0xffffffff, 100, 0);
    break;
  }

  case OPCODE_SLEEP_MODE_CONFIGURATIONS :
    *p << OPCODE_READ_BD_ADDR;
    //p->command(OPCODE_READ_BD_ADDR);
    break;

  case OPCODE_READ_BD_ADDR :
    *p >> bd_addr;
    //p->fget("b", &bd_addr);

    UARTprintf("bd_addr = ");
    for (int i=5; i >= 0; --i) UARTprintf("%02x:", bd_addr.data[i]);
    UARTprintf("\n");

    *p << OPCODE_READ_BUFFER_SIZE_COMMAND;
    //p->command(OPCODE_READ_BUFFER_SIZE_COMMAND);
    break;

  case OPCODE_READ_BUFFER_SIZE_COMMAND : {
    uint16_t acl_data_length, num_acl_packets, num_synchronous_packets;
    uint8_t synchronous_data_length;

    *p >> acl_data_length >> synchronous_data_length;
    *p >> num_acl_packets >> num_synchronous_packets;

    // p->fget("2122", &acl_data_length, &synchronous_data_length,
    //                    &num_acl_packets, &num_synchronous_packets);

    UARTprintf("acl: %d @ %d, synchronous: %d @ %d\n",
               num_acl_packets, acl_data_length,
               num_synchronous_packets, synchronous_data_length);

    *p << OPCODE_WRITE_PAGE_TIMEOUT << (uint16_t) 0x2000;
    //p->command(OPCODE_WRITE_PAGE_TIMEOUT, "2", 0x2000);
    break;
  }

  case OPCODE_WRITE_PAGE_TIMEOUT :
    *p << OPCODE_READ_PAGE_TIMEOUT;
    //p->command(OPCODE_READ_PAGE_TIMEOUT);
    break;

  case OPCODE_READ_PAGE_TIMEOUT : {
    uint16_t timeout;

    *p >> timeout;
    //p->fget("2", &timeout);
    uint32_t usec = timeout*625;
    UARTprintf("page timeout = %d.%d msec\n", usec/1000, usec%1000);

    *p << OPCODE_WRITE_LOCAL_NAME_COMMAND << (local_name_ptr) "Super Whizzy Gizmo 1.0";
    //p->command(OPCODE_WRITE_LOCAL_NAME_COMMAND, "n", "Super Whizzy Gizmo 1.0");
    break;
  }

  case OPCODE_WRITE_LOCAL_NAME_COMMAND :
    UARTprintf("local name set\n");
    *p << OPCODE_WRITE_SCAN_ENABLE << (uint8_t) 0x03;
    //p->command(OPCODE_WRITE_SCAN_ENABLE, "1", 0x03);
    break;

  case OPCODE_WRITE_SCAN_ENABLE :
    UARTprintf("local scans enabled\n");
    // http://bluetooth-pentest.narod.ru/software/bluetooth_class_of_device-service_generator.html
    *p << OPCODE_WRITE_CLASS_OF_DEVICE << (uint32_t) 0x0098051c;
    // p->command(OPCODE_WRITE_CLASS_OF_DEVICE, "4", 0x0098051c);
    break;

  case OPCODE_WRITE_CLASS_OF_DEVICE :
    UARTprintf("device class set\n");
    *p << OPCODE_SET_EVENT_MASK << (uint32_t) 0xffffffff << (uint32_t) 0x20001fff;
    // p->command(OPCODE_SET_EVENT_MASK, "44", 0xffffffff, 0x20001fff);
    break;

  case OPCODE_SET_EVENT_MASK :
    UARTprintf("event mask set\n");
    *p << OPCODE_WRITE_LE_HOST_SUPPORT << (uint8_t) 1 << (uint8_t) 1;
    //p->reset();
    //p->command(OPCODE_WRITE_LE_HOST_SUPPORT, "11", 1, 1);
    break;

  case OPCODE_WRITE_LE_HOST_SUPPORT :
    UARTprintf("host support written\n");
    *p << OPCODE_LE_SET_EVENT_MASK << (uint32_t) 0x0000001f << (uint32_t) 0x00000000;
    //p->command(OPCODE_LE_SET_EVENT_MASK, "44", 0x0000001f, 0x00000000);
    break;

  case OPCODE_LE_SET_EVENT_MASK :
    UARTprintf("event mask set\n");
    *p << OPCODE_LE_READ_BUFFER_SIZE;
    //p->command(OPCODE_LE_READ_BUFFER_SIZE);
    break;

  case OPCODE_LE_READ_BUFFER_SIZE : {
    UARTprintf("buffer size read\n");
    uint16_t le_data_packet_length;
    uint8_t num_le_packets;

    *p >> le_data_packet_length >> num_le_packets;
    //p->fget("21", &le_data_packet_length, &num_le_packets);
    UARTprintf("le_data_packet_length = %d, num_packets = %d\n", le_data_packet_length, num_le_packets);

    *p << OPCODE_LE_READ_SUPPORTED_STATES;
    //p->command(OPCODE_LE_READ_SUPPORTED_STATES);
    break;
  }

  case OPCODE_LE_READ_SUPPORTED_STATES : {
    UARTprintf("supported states read\n");

    uint64_t states;
    *p >> states;
    //p->fget("8", states);
    UARTprintf("states = 0x");
    for (int i=0; i < 8; ++i) UARTprintf("%02x", ((uint8_t *) &states)[i]);
    UARTprintf("\n");

    *p << OPCODE_LE_SET_ADVERTISING_PARAMETERS << (uint16_t) 0x0800 << (uint16_t) 0x4000;
    *p << (uint8_t) 0 << (uint8_t) 0 << (uint8_t) 0;
    *p << bd_addr << (uint8_t) 0x07 << (uint8_t) 0;

    //p->command(OPCODE_LE_SET_ADVERTISING_PARAMETERS, "22111b11",
    //               0x0800, 0x4000, 0, 0, 0, &bd_addr, 0x07, 0);
    break;
  }
    
  case OPCODE_LE_SET_ADVERTISING_PARAMETERS : {
    UARTprintf("advertising parameters set\n");
    // https://www.bluetooth.org/Technical/AssignedNumbers/generic_access_profile.htm
    // AD #1, data type (0x01) <<Flags>>
    // AD #2, data type (0x02) <<Incomplete list of 16-bit service UUIDs>>

    /*
    uint8_t num_parameters = 0;
    uint8_t advertising_data[] = {02, 01, 05, 03, 02, 0xf0, 0xff, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0};
    */

    *p << OPCODE_LE_SET_ADVERTISING_DATA;
    assert(p->get_remaining() > 31);
    Packet adv(p->ptr(), 31); // a piece of the main packet
    uint8_t *start;

    // data item 1
    start = adv.ptr();
    adv << (uint8_t) 0; // dummy length
    adv << (uint8_t) GAP::FLAGS;
    adv << (uint8_t) (GAP::LE_LIMITED_DISCOVERABLE_MODE | GAP::BR_EDR_NOT_SUPPORTED);
    *start = (adv.ptr() - start) - 1;

    // data item 2
    start = adv.ptr();
    adv << (uint8_t) 0; // dummy length
    adv << (uint8_t) GAP::INCOMPLETE_16BIT_UUIDS;
    adv << (uint16_t) 0xfff0; // demo UUID not in the Bluetooth spec
    *start = (adv.ptr() - start) - 1;

    // zero pad to 31 bytes
    while (adv.get_position() < 31) adv << (uint8_t) 0;
    assert(adv.get_position() == 31);

    p->skip(31);
    break;
  }
    
  case OPCODE_LE_SET_ADVERTISING_DATA : {
    UARTprintf("advertising data set\n");

    *p << OPCODE_LE_SET_SCAN_RESPONSE_DATA;
    assert(p->get_remaining() > 31);
    Packet adv(p->ptr(), 31); // a piece of the main packet
    uint8_t *start;

    // data item 1
    start = adv.ptr();
    adv << (uint8_t) 0; // dummy length
    adv << (uint8_t) GAP::FLAGS;
    adv << (uint8_t) (GAP::LE_LIMITED_DISCOVERABLE_MODE | GAP::BR_EDR_NOT_SUPPORTED);
    *start = (adv.ptr() - start) - 1;

    // data item 2
    start = adv.ptr();
    adv << (uint8_t) 0; // dummy length
    adv << (uint8_t) GAP::INCOMPLETE_16BIT_UUIDS;
    adv << (uint16_t) 0xfff0; // demo UUID not in the Bluetooth spec
    *start = (adv.ptr() - start) - 1;

    // zero pad to 31 bytes
    while (adv.get_position() < 31) adv << (uint8_t) 0;
    assert(adv.get_position() == 31);

    p->skip(31);
    break;
  }
    
  case OPCODE_LE_SET_SCAN_RESPONSE_DATA :
    UARTprintf("response data set\n");
    *p << OPCODE_LE_SET_ADVERTISE_ENABLE << (uint8_t) 0x01;
    break;

  case OPCODE_LE_SET_ADVERTISE_ENABLE :
    UARTprintf("warm boot finished\n");

    deallocate_packet(p);
    p = 0;
    break;

  default :
    UARTprintf("discarding unrecognized event packet (%02x) in during cold boot\n", opcode);
    deallocate_packet(p);
    p = 0;
  }

  if (p) send(p);
}

