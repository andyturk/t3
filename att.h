#pragma once

#include <stdint.h>

#include "l2cap.h"
#include "ring.h"
#include "packet.h"

class AttributeBase : public Ring<AttributeBase> {
  enum {MAX_ATTRIBUTES = 20};
  static uint16_t next_handle;
  static AttributeBase *all_handles[MAX_ATTRIBUTES];

 public:
  UUID type;
  uint16_t handle;
  void *_data;
  uint16_t length;

  AttributeBase(const UUID &t, void *d, uint16_t l);
  AttributeBase(int16_t t, void *d, uint16_t l);

  static AttributeBase *get(uint16_t h) {return (h < next_handle) ? all_handles[h] : 0;}
  static uint16_t find_by_type_value(uint16_t start, uint16_t type, void *value, uint16_t length);
  static uint16_t find_by_type(uint16_t start, const UUID &type);
  int compare(void *data, uint16_t len);
  int compare(void *data, uint16_t len, uint16_t min_handle, uint16_t max_handle);
  virtual uint16_t group_end();

  static void dump_attributes();
};

template<typename T>
class Attribute : public AttributeBase {
  T data;

 public:
  Attribute(const UUID &u) : AttributeBase(u, &data, sizeof(data)) {}
  Attribute(uint16_t u) : AttributeBase(u, &data, sizeof(data)) {}
  Attribute(const UUID &u, const T &val) : AttributeBase(u, &data, sizeof(data)), data(val) {}
  Attribute(uint16_t u, const T &val) : AttributeBase(u, &data, sizeof(data)), data(val) {}

  Attribute &operator=(const T &rhs) {data = rhs; return *this;};
};

template<>
class Attribute<const char *> : public AttributeBase {
 public:
  Attribute(const UUID &u) : AttributeBase(u, 0, 0) {}
  Attribute(uint16_t u) : AttributeBase(u, 0, 0) {}

  Attribute &operator=(const char *rhs) {_data = (void *) rhs; length = strlen(rhs); return *this;};
};

class ATT_Channel : public Channel {
  bool read_handles(uint16_t &starting, uint16_t &ending, Packet *p, uint8_t opcode);
  bool read_uuid(UUID &u, Packet *p, uint8_t opcode);
  void error(uint8_t err, Packet *p, uint16_t handle, uint8_t opcode);
  bool is_grouping(const UUID &type);

  void find_by_type_value(uint16_t from, uint16_t to, UUID &type, Packet *p);
  void read_by_type(uint16_t from, uint16_t to, UUID &type, Packet *p);
  void read_by_group_type(uint16_t from, uint16_t to, UUID &type, Packet *p);
  void read(uint16_t h, Packet *p);
  void read_blob(uint16_t h, uint16_t offset, Packet *p);

  Ring<AttributeBase> attributes;
  uint16_t att_mtu;
  
 public:
  ATT_Channel(HostController &hc);
  void receive(Packet *p);
  void add(AttributeBase &attr) { attr.join(&attributes); }
};


