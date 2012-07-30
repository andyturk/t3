#pragma once

#include <stdint.h>
#include <stdarg.h>
#include <deque>

#include "cc_stubs.h"
#include "hal.h"
#include "buffer.h"
#include "pool.h"
#include "ring.h"
#include "uuid.h"
#include "bluetooth_constants.h"
#include "bd_addr.h"
#include "packet.h"
#include "h4.h"
#include "bts.h"

extern const char hex_digits[16];

namespace HCI {
  class Connection : public Ring<Connection> {
    uint16_t handle;
  };
};

using namespace HCI;

class HostController : public H4Controller {
 protected:
  PoolBase<Packet> *command_packets;
  PoolBase<Packet> *acl_packets;
  PoolBase<Connection> *connections;

  Ring<Connection> remotes;
  uint8_t command_packet_budget;

 public:
  BD_ADDR bd_addr;

  HostController(PoolBase<Packet> *cmd, PoolBase<Packet> *acl, PoolBase<Connection> *conn) :
    command_packets(cmd),
    acl_packets(acl),
    connections(conn)
  {}

  // H4Controller methods
  virtual void sent(Packet *p) {}
  virtual void received(Packet *p) {}

  virtual void initialize() {}
  virtual void periodic(uint32_t msec) {}
  virtual void send(Packet *p);
  virtual void receive(Packet *p) {
    // p->join(&incoming_packets);
  }

  friend class H4Tranceiver;
  friend class ATT_Channel;
};

class BBand : public HostController {
  UART &uart;
  IOPin &shutdown;
  Pool<HCI::Connection, 3> hci_connection_pool;
  BTS::H4Script *script;

  void (*event_handler)(BBand *, uint8_t event, Packet *);
  void (*command_complete_handler)(BBand *, uint16_t opcode, Packet *);

  // initialization states
  void cold_boot(uint16_t opcode, Packet *p);
  void upload_patch(uint16_t opcode, Packet *p);
  void warm_boot(uint16_t opcode, Packet *p);
  void normal_operation(uint16_t opcode, Packet *p);
  void command_complete(uint16_t opcode, Packet *p);
  
  void standard_packet_handler(Packet *p);
  void default_event_handler(uint8_t event, Packet *p);
  void acl_packet_handler(uint16_t handle, uint8_t pb, uint8_t bc, Packet *p);
  void l2cap_packet_handler(Packet *p);
  void le_event_handler(uint8_t subevent, Packet *p);
  void att_packet_handler(Packet *p);

  struct {
    uint16_t expected_opcode;
    size_t offset;
    size_t length;
    const uint8_t *data;
  } patch_state;

 public:
  BBand(UART &u, IOPin &s);
  void initialize();
  void process_incoming_packets();
};
