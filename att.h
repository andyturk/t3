#pragma once

#include <stdint.h>

#include "l2cap.h"
#include "ring.h"
#include "packet.h"

class ATT_Channel : public Channel {
  bool read_handles(uint16_t &starting, uint16_t &ending, Packet *p, uint8_t opcode);
  bool read_uuid(UUID &u, Packet *p, uint8_t opcode);
  void attribute_not_found(Packet *p, uint16_t handle, uint8_t opcode);
  bool is_grouping(const UUID &type);

  void find_by_type_value(uint16_t from, uint16_t to, UUID &type, Packet *p);
  void read_by_type(uint16_t from, uint16_t to, UUID &type, Packet *p);
  void read_by_group_type(uint16_t from, uint16_t to, UUID &type, Packet *p);

  Ring<AttributeBase> attributes;
  uint16_t att_mtu;
  
 public:
  ATT_Channel(HostController &hc);
  void receive(Packet *p);
  void add(AttributeBase &attr) { attr.join(&attributes); }
};


