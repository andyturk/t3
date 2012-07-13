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

extern const char hex_digits[16];

namespace HCI {
  class Connection : public Ring<Connection> {
    uint16_t handle;
  };
};

using namespace HCI;

class AttributeBase : public Ring<AttributeBase> {
  enum {MAX_ATTRIBUTES = 20};
  static uint16_t next_handle;
  static AttributeBase *all_handles[MAX_ATTRIBUTES];

 public:
  UUID type;
  uint16_t handle;
  void *_data;
  uint16_t length;

 AttributeBase(const UUID &t, void *d, uint16_t l) :
  type(t), handle(++next_handle), _data(d), length(l)
  {
    all_handles[handle] = this;
  }
 AttributeBase(int16_t t, void *d, uint16_t l) :
    type(t), handle(++next_handle), _data(d), length(l)
  {
    all_handles[handle] = this;
  }

  static AttributeBase *get(uint16_t n) {assert(n < next_handle); return all_handles[n];}
  static uint16_t find_by_type_value(uint16_t start, uint16_t type, void *value, uint16_t length) {
    assert(start > 0);
    for (uint16_t i=start; i < next_handle; ++i) {
      AttributeBase *attr = all_handles[i];

      if (attr->type != type) continue;
      if (length != attr->length) continue; // e.g., UUIDs could be either 2 or 16 bytes
      if (memcmp(attr->_data, value, length)) continue;

      return i;
    }

    return 0;
  }
  static uint16_t find_by_type(uint16_t start, const UUID &type) {
    assert(start > 0);
    for (uint16_t i=start; i < next_handle; ++i) {
      if (all_handles[i]->type == type) return i;
    }
    return 0;
  }
  int compare(void *data, uint16_t len);
  int compare(void *data, uint16_t len, uint16_t min_handle, uint16_t max_handle);
  virtual uint16_t group_end();
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

struct CharacteristicDecl : public AttributeBase {
  struct {
    uint8_t properties;
    uint16_t handle;
    union {
      uint8_t full_uuid[sizeof(UUID)];
      uint16_t short_uuid;
    };
  } __attribute__ ((packed)) _decl ;

  CharacteristicDecl(uint16_t uuid) :
    AttributeBase(GATT::CHARACTERISTIC, &_decl, sizeof(_decl))
  {
    _decl.properties = 0;
    _decl.handle = 0;
    _decl.short_uuid = (uint16_t) uuid;
    length = sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint16_t);
  }

  CharacteristicDecl(const UUID &uuid) :
    AttributeBase(GATT::CHARACTERISTIC, &_decl, sizeof(_decl))
  {
    _decl.properties = 0;
    _decl.handle = 0;
    memcpy(_decl.full_uuid, (const uint8_t *) uuid, sizeof(UUID));
    length = sizeof(uint8_t) + sizeof(uint16_t) + sizeof(UUID);
  }
};

template<typename T>
struct Characteristic : public CharacteristicDecl {
  Attribute<T> value;
  Characteristic(const UUID &uuid) :
    CharacteristicDecl(GATT::CHARACTERISTIC),
    value(uuid)
  {
    _decl.handle = value.handle;
  }
  Characteristic(uint16_t uuid) :
    CharacteristicDecl(GATT::CHARACTERISTIC),
    value(uuid)
  {
    _decl.handle = value.handle;
  }
    Characteristic &operator=(const T &rhs) {value = rhs; return *this;}
};


struct GATT_Service : public Attribute<uint16_t> {
  CharacteristicDecl changed;

  GATT_Service() :
    Attribute<uint16_t>(GATT::PRIMARY_SERVICE, GATT::GENERIC_ATTRIBUTE_PROFILE),
      changed(GATT::SERVICE_CHANGED)
  {
    changed._decl.properties = GATT::INDICATE | GATT::WRITE_WITHOUT_RESPONSE | GATT::READ;
  }
  virtual uint16_t group_end() {return changed.handle;}
};

struct MyService : public Attribute<uint16_t> {
  Characteristic<char> char_1;
  Characteristic<char> char_2;
  Characteristic<char> char_3;

  MyService() :
    Attribute<uint16_t>(GATT::PRIMARY_SERVICE, 0xfff0),
      char_1((uint16_t) 0xfff1),
      char_2((uint16_t) 0xfff2),
      char_3("00001234-0000-1000-8000-00805F9B34FB")
  {
  }

  virtual uint16_t group_end() {return char_3.handle;}
};

struct GAP_Service : public Attribute<uint16_t> {
  Characteristic<const char *> device_name;
  Characteristic<uint16_t> appearance;

  GAP_Service(const char *name, uint16_t a = 0) :
    Attribute<uint16_t>(GATT::PRIMARY_SERVICE, GATT::GENERIC_ACCESS_PROFILE),
      device_name(GATT::DEVICE_NAME),
      appearance(GATT::APPEARANCE)
  {
    appearance = a;
    device_name = name;
  }
  virtual uint16_t group_end() {return appearance.handle;}
};

class HostController {
 protected:
  PoolBase<Packet> *command_packets;
  PoolBase<Packet> *acl_packets;
  PoolBase<Connection> *connections;

  Ring<Packet> incoming_packets;
  Ring<Packet> sent;
  Ring<Connection> remotes;
  uint8_t command_packet_budget;

 public:
  BD_ADDR bd_addr;

  HostController(PoolBase<Packet> *cmd, PoolBase<Packet> *acl, PoolBase<Connection> *conn) :
    command_packets(cmd),
    acl_packets(acl),
    connections(conn)
  {}

  virtual void initialize() {}
  virtual void periodic(uint32_t msec) {}
  virtual void send(Packet *p);
  virtual void receive(Packet *p) {
    p->join(&incoming_packets);
  }

  friend class H4Tranceiver;
  friend class ATT_Channel;
};


class BBand : public HostController {
  UART &uart;
  IOPin &shutdown;
  PacketPool<259, 4> command_packet_pool;
  PacketPool<1000, 4> acl_packet_pool;
  Pool<HCI::Connection, 3> hci_connection_pool;

  void (*event_handler)(BBand *, uint8_t event, Packet *);
  void (*command_complete_handler)(BBand *, uint16_t opcode, Packet *);

  GAP_Service gap;
  GATT_Service gatt;
  MyService my;

  // initialization states
  void cold_boot(uint16_t opcode, Packet *p);
  void upload_patch(uint16_t opcode, Packet *p);
  void warm_boot(uint16_t, Packet *p);

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
