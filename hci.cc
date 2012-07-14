#include "hci.h"
#include "att.h"
#include "l2cap.h"
#include "cc_stubs.h"
#include "h4.h"
#include "assert.h"

using namespace HCI;

const char hex_digits[16] = {
  '0', '1', '2', '3', '4', '5', '6', '7',
  '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

char BD_ADDR::pp_buf[24];

uint16_t AttributeBase::next_handle = 0;
AttributeBase *AttributeBase::all_handles[AttributeBase::MAX_ATTRIBUTES];

int AttributeBase::compare(void *other, uint16_t len) {
  size_t shorter = length < len ? length : len;
  int c = memcmp(_data, other, shorter);
  if (c != 0) return c;
  if (len == length) return c;
  return length < len ? -1 : 1;
}

BBand::BBand(UART &u, IOPin &s) :
  HostController((PoolBase<Packet> *) &command_packet_pool,
                 (PoolBase<Packet> *) &acl_packet_pool,
                 (PoolBase<HCI::Connection> *) &hci_connection_pool),
  uart(u),
  shutdown(s),
  event_handler(&default_event_handler)
{
}

void BBand::initialize() {
  extern H4Tranceiver h4;

  shutdown.set_value(0); // assert SHUTDOWN
  uart.set_enable(false);
  uart.set_fifo_enable(false);
  uart.set_fifo_enable(true);
  uart.set_baud(115200);
  uart.set_enable(true);

  h4.reset();

  uart.set_interrupt_sources(UART::RX | UART::ERROR);
  command_complete_handler = &cold_boot;

  shutdown.set_value(1); // clear SHUTDOWN
  uart.set_interrupt_enable(true);

  CPU::delay(150); // wait 150 msec
  cold_boot(0, 0);
}

void HostController::send(Packet *p) {
  extern H4Tranceiver h4;
  assert(p != 0);
  p->prepare_for_tx();
  p->join(&sent);
  h4.fill_uart();
}

void BBand::process_incoming_packets() {
  Packet *p;

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

void BBand::default_event_handler(uint8_t event, Packet *p) {
  switch (event) {
  case EVENT_DISCONNECTION_COMPLETE : {
    uint16_t handle;
    uint8_t status, reason;

    *p >> status >> handle >> reason;
    handle &= 0x0fff;

    printf("disconnected 0x%04x (with status: 0x%02x) because 0x%02x\n", handle, status, reason);
    printf("re-enabling LE advertising\n");
    p->hci(OPCODE_LE_SET_ADVERTISE_ENABLE) << (uint8_t) 0x01;
    send(p);
    return;
  }
    
  case EVENT_LE_META_EVENT : {
    uint8_t subevent;
    *p >> subevent;
    le_event_handler(subevent, p);
    break;
  }

  case EVENT_NUMBER_OF_COMPLETED_PACKETS : {
    uint8_t number_of_handles;

    *p >> number_of_handles;
    uint16_t *connection_handle = (uint16_t *) (uint8_t *) *p;
    p->skip(number_of_handles*sizeof(uint16_t));
    uint8_t *num_of_completed_packets = (uint8_t *) *p;

    for (uint8_t i=0; i < number_of_handles; ++i) {
      printf("connection 0x%04x completed %d packets\n",
                 connection_handle[i],
                 num_of_completed_packets[i]);
    }
    break;
  }
  default :
    printf("discarding event 0x%02x\n", event);
  }

  p->deallocate();
}

void BBand::le_event_handler(uint8_t subevent, Packet *p) {
  switch(subevent) {
  case LE_EVENT_CONNECTION_COMPLETE : {
    BD_ADDR peer_address;
    uint8_t status, role, peer_address_type, master_clock_accuracy;
    uint16_t connection_handle, conn_interval, conn_latency, supervision_timeout;

    *p >> status >> connection_handle >> role;
    *p >> peer_address_type;
    p->write(peer_address.data, sizeof(peer_address.data));
    *p >> conn_interval >> conn_latency;
    *p >> supervision_timeout >> master_clock_accuracy;

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

    const char *addr = peer_address.pretty_print();
    printf("connection to %s completed with status %d\n", addr, status);
    printf("  handle = 0x%04x, addr_type = %s\n", connection_handle, type);
    printf("  role = %s, interval = %d, latency = %d, timeout = %d\n", role_name, conn_interval, conn_latency, supervision_timeout);
    printf("  clock_accuracy = %d\n", master_clock_accuracy);
    break;
  }
  case LE_EVENT_ADVERTISING_REPORT :
  case LE_EVENT_CONNECTION_UPDATE_COMPLETE :
  case LE_EVENT_READ_REMOTE_USED_FEATURES_COMPLETE :
  case LE_EVENT_LONG_TERM_KEY_REQUEST :
  default :
    printf("ignoring unrecognized LE event: 0x%02x\n", subevent);
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
    p->skip(3*sizeof(uint16_t)); // acl handle, acl length, l2cap length
    uint16_t cid;

    *p >> cid;
    Channel *channel = Channel::find(cid);

    if (channel) {
      printf("ACL packet for channel 0x%04x\n", cid);
      channel->receive(p);
    } else {
      printf("discarding ACL packet for unknown channel: 0x%04x\n", cid);
      p->deallocate();
    }
    break;
  }

  case COMMAND_PACKET :
  case SYNCHRONOUS_DATA_PACKET :
  default :
    printf("discarding unknown packet of type %d\n", packet_indicator);
    p->deallocate();
  }
}

void BBand::cold_boot(uint16_t opcode, Packet *p) {
  if (p != 0) {
    uint8_t status = p->get();
    assert(status == SUCCESS);
  }

  switch (opcode) {
  case 0 :
    // This case runs before the uart is enabled, so we simply
    // format the tx packet and set p=0 to prevent it from
    // being sent at this time. Later on, the calling code will
    // send the contents of the tx buffer through the uart.
    assert(p == 0);
    p = command_packets->allocate();
    p->hci(OPCODE_RESET);
    break;

  case OPCODE_RESET :
    p->hci(OPCODE_READ_LOCAL_VERSION_INFORMATION);
    break;

  case OPCODE_READ_LOCAL_VERSION_INFORMATION : {
    uint8_t hci_version, lmp_version;
    uint16_t hci_revision, manufacturer_name, lmp_subversion;

    *p >> hci_version >> hci_revision >> lmp_version >> manufacturer_name >> lmp_subversion;
    assert(hci_version == SPECIFICATION_4_0);

    p->hci(OPCODE_PAN13XX_CHANGE_BAUD_RATE) << (uint32_t) 921600L;
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
    printf("discarding unrecognized event packet (%02x) in during cold boot\n", opcode);
    p->deallocate();
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
    //printf("patch command %04x @ %d (%d)\n",
    //           patch_state.expected_opcode, patch_state.offset - command_length, command_length);
    p->flip();
    send(p);
  } else {
    printf("patching complete\n");
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

void BBand::normal_operation(uint16_t opcode, Packet *p) {
  printf("OK 0x%04x\n", opcode);
  p->deallocate();
}

void BBand::warm_boot(uint16_t opcode, Packet *p) {
  assert(!uart.can_read());
  if (opcode != 0) {
    uint8_t status = p->get();

    if (status != SUCCESS) {
      uint16_t ocf = opcode & 0x03ff;
      uint16_t ogf = opcode >> 10;

      printf("bad command status (0x%x): ogf: %d ocf: 0x%04x\n", status, ogf, ocf);
      for (;;);
    }
  }

  switch (opcode) {
  case 0 : {
    printf("warm boot...\n");
    uint8_t deep_sleep_enable = 0x00;
    p->hci(OPCODE_SLEEP_MODE_CONFIGURATIONS) << (uint8_t) 0x01 << deep_sleep_enable;
    *p << (uint8_t) 0 << (uint32_t) 0xffffffff << (uint8_t) 100 << (uint8_t) 0;
    //p->command(OPCODE_SLEEP_MODE_CONFIGURATIONS, "111411", 0x01, deep_sleep_enable, 0, 0xffffffff, 100, 0);
    break;
  }

  case OPCODE_SLEEP_MODE_CONFIGURATIONS :
    p->hci(OPCODE_READ_BD_ADDR);
    //p->command(OPCODE_READ_BD_ADDR);
    break;

  case OPCODE_READ_BD_ADDR :
    p->read(bd_addr.data, sizeof(bd_addr.data));

    printf("bd_addr = ");
    for (int i=5; i >= 0; --i) printf("%02x:", bd_addr.data[i]);
    printf("\n");

    p->hci(OPCODE_READ_BUFFER_SIZE_COMMAND);
    break;

  case OPCODE_READ_BUFFER_SIZE_COMMAND : {
    uint16_t acl_data_length, num_acl_packets, num_synchronous_packets;
    uint8_t synchronous_data_length;

    *p >> acl_data_length >> synchronous_data_length;
    *p >> num_acl_packets >> num_synchronous_packets;

    // p->fget("2122", &acl_data_length, &synchronous_data_length,
    //                    &num_acl_packets, &num_synchronous_packets);

    printf("acl: %d @ %d, synchronous: %d @ %d\n",
               num_acl_packets, acl_data_length,
               num_synchronous_packets, synchronous_data_length);

    p->hci(OPCODE_WRITE_PAGE_TIMEOUT) << (uint16_t) 0x2000;
    //p->command(OPCODE_WRITE_PAGE_TIMEOUT, "2", 0x2000);
    break;
  }

  case OPCODE_WRITE_PAGE_TIMEOUT :
    p->hci(OPCODE_READ_PAGE_TIMEOUT);
    //p->command(OPCODE_READ_PAGE_TIMEOUT);
    break;

  case OPCODE_READ_PAGE_TIMEOUT : {
    uint16_t timeout;

    *p >> timeout;
    //p->fget("2", &timeout);
    uint32_t usec = timeout*625;
    printf("page timeout = %d.%d msec\n", usec/1000, usec%1000);

    char local_name[248];
    strncpy(local_name, "Super Whizzy Gizmo 1.0", sizeof(local_name));

    p->hci(OPCODE_WRITE_LOCAL_NAME_COMMAND);
    p->write((uint8_t *) local_name, sizeof(local_name));
    //*p << OPCODE_WRITE_LOCAL_NAME_COMMAND << (local_name_ptr) "Super Whizzy Gizmo 1.0";
    //p->command(OPCODE_WRITE_LOCAL_NAME_COMMAND, "n", "Super Whizzy Gizmo 1.0");
    break;
  }

  case OPCODE_WRITE_LOCAL_NAME_COMMAND :
    printf("local name set\n");
    p->hci(OPCODE_WRITE_SCAN_ENABLE) << (uint8_t) 0x03;
    //p->command(OPCODE_WRITE_SCAN_ENABLE, "1", 0x03);
    break;

  case OPCODE_WRITE_SCAN_ENABLE :
    printf("local scans enabled\n");
    // http://bluetooth-pentest.narod.ru/software/bluetooth_class_of_device-service_generator.html
    p->hci(OPCODE_WRITE_CLASS_OF_DEVICE) << (uint32_t) 0x0098051c;
    // p->command(OPCODE_WRITE_CLASS_OF_DEVICE, "4", 0x0098051c);
    break;

  case OPCODE_WRITE_CLASS_OF_DEVICE :
    printf("device class set\n");
    p->hci(OPCODE_SET_EVENT_MASK) << (uint32_t) 0xffffffff << (uint32_t) 0x20001fff;
    // p->command(OPCODE_SET_EVENT_MASK, "44", 0xffffffff, 0x20001fff);
    break;

  case OPCODE_SET_EVENT_MASK :
    printf("event mask set\n");
    p->hci(OPCODE_WRITE_LE_HOST_SUPPORT) << (uint8_t) 1 << (uint8_t) 1;
    //p->reset();
    //p->command(OPCODE_WRITE_LE_HOST_SUPPORT, "11", 1, 1);
    break;

  case OPCODE_WRITE_LE_HOST_SUPPORT :
    printf("host support written\n");
    p->hci(OPCODE_LE_SET_EVENT_MASK) << (uint32_t) 0x0000001f << (uint32_t) 0x00000000;
    //p->command(OPCODE_LE_SET_EVENT_MASK, "44", 0x0000001f, 0x00000000);
    break;

  case OPCODE_LE_SET_EVENT_MASK :
    printf("event mask set\n");
    p->hci(OPCODE_LE_READ_BUFFER_SIZE);
    break;

  case OPCODE_LE_READ_BUFFER_SIZE : {
    printf("buffer size read\n");
    uint16_t le_data_packet_length;
    uint8_t num_le_packets;

    *p >> le_data_packet_length >> num_le_packets;
    printf("le_data_packet_length = %d, num_packets = %d\n", le_data_packet_length, num_le_packets);

    p->hci(OPCODE_LE_READ_SUPPORTED_STATES);
    break;
  }

  case OPCODE_LE_READ_SUPPORTED_STATES : {
    printf("supported states read\n");

    uint8_t states[8];
    p->read(states, sizeof(states));
    printf("states = 0x");
    for (int i=0; i < 8; ++i) printf("%02x", states[i]);
    printf("\n");

    p->hci(OPCODE_LE_SET_ADVERTISING_PARAMETERS) << (uint16_t) 0x0800 << (uint16_t) 0x4000;
    *p << (uint8_t) 0 << (uint8_t) 0 << (uint8_t) 0;
    p->write(bd_addr.data, sizeof(bd_addr.data)) << (uint8_t) 0x07 << (uint8_t) 0;
    break;
  }
    
  case OPCODE_LE_SET_ADVERTISING_PARAMETERS : {
    printf("advertising parameters set\n");
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

    p->hci(OPCODE_LE_SET_ADVERTISING_DATA) << (uint8_t) 0;
    assert(p->get_remaining() > 31);
    Packet adv(p->ptr(), 31); // a piece of the main packet
    uint8_t *start0, *start;

    // data item 1
    start = start0 = adv.ptr();
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

    *start0 = adv.get_position();
    p->skip(31);
    break;
  }
    
  case OPCODE_LE_SET_ADVERTISING_DATA : {
    printf("advertising data set\n");

    p->hci(OPCODE_LE_SET_SCAN_RESPONSE_DATA) << (uint8_t) 0;
    assert(p->get_remaining() > 31);
    Packet adv(p->ptr(), 31); // a piece of the main packet
    uint8_t *start0, *start;

    // data item 1
    start = start0 = adv.ptr();
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

    *start0 = adv.get_position();

    p->skip(31);
    break;
  }
    
  case OPCODE_LE_SET_SCAN_RESPONSE_DATA :
    printf("response data set\n");
    p->hci(OPCODE_LE_SET_ADVERTISE_ENABLE) << (uint8_t) 0x01;
    break;

  case OPCODE_LE_SET_ADVERTISE_ENABLE :
    printf("warm boot finished\n");
    command_complete_handler = &normal_operation;

    p->deallocate();
    p = 0;
    break;

  default :
    printf("discarding unrecognized event packet (%02x) in during cold boot\n", opcode);
    p->deallocate();
    p = 0;
  }

  if (p) send(p);
}

