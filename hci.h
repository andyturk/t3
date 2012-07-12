#pragma once

#include <stdint.h>
#include <stdarg.h>
#include <deque>

#include "cc_stubs.h"
#include "hal.h"
#include "buffer.h"
#include "ring.h"
#include "uuid.h"
#include "bluetooth_constants.h"

extern const char hex_digits[16];

namespace HCI {
  struct BD_ADDR {
    enum {PP_BUF_LEN = 24};
    uint8_t data[6];
    void pretty_print(char *buf) {
      for (int i=5; i >= 0; --i) {
        *buf++ = hex_digits[data[i] >> 4];
        *buf++ = hex_digits[data[i] & 0x0f];
        if (i > 0) *buf++ = ':';
      }
      *buf++ = 0;
    }
  };

  typedef char *local_name_ptr;
  typedef char local_name[248];
  extern "C" void * memcpy(void * dst, void const * src, size_t len);

  class Packet : public Ring<Packet>, public FlipBuffer<uint8_t> {
    //void vfput(const char *format, va_list args);
    
  public:
    Packet(uint8_t *buf, size_t len) : FlipBuffer<uint8_t>(buf, len) {}

    void read(uint8_t *p, size_t len) {
      assert(position + len <= limit);
      memcpy(p, storage + position, len);
      position += len;
    }

    void write(const uint8_t *p, size_t len) {
      assert(position + len <= limit);
      memcpy(storage + position, p, len);
      position += len;
    }

    Packet &operator<<(uint8_t x) {
      put(x);
      return *this;
    }

    Packet &operator<<(uint16_t x) {
      put(x & 0x00ff);
      put(x >> 8);
      return *this;
    }

    Packet &operator<<(HCI::opcode x) {
      begin_hci(COMMAND_PACKET);
      return *this << (uint16_t) x << (uint8_t) 0;
    }

    Packet &operator<<(uint32_t x) {
      put((x >>  0) & 0x000000ff);
      put((x >>  8) & 0x000000ff);
      put((x >> 16) & 0x000000ff);
      put((x >> 24) & 0x000000ff);
      return *this;
    }

    Packet &operator<<(const uint64_t &x) {
      // assume little endian
      const uint8_t *p = (const uint8_t *) &x;
      do {put(*p++);} while (p < (uint8_t *) ((&x)+1));
      return *this;
    }

    Packet &operator<<(const BD_ADDR &addr) {
      write((uint8_t *) &addr, sizeof(BD_ADDR));
      return *this;
    }

    Packet &operator<<(local_name_ptr name) {
      unsigned int i=0;
      while (i < sizeof(local_name) && name[i] != 0) put(name[i++]);
      while (i < sizeof(local_name)) {put(0); ++i;}
      return *this;
    }

    Packet &operator>>(uint8_t &x) {
      x = get();
      return *this;
    }

    Packet &operator>>(uint16_t &x) {
      x = get() + (get() << 8);
      return *this;
    }

    Packet &operator>>(uint32_t &x) {
      x = get() + (get() << 8) + (get() << 16) + (get() << 24);
      return *this;
    }

    Packet &operator>>(uint64_t &x) {
      // assume little endian
      uint8_t *p = (uint8_t *) &x;
      do {*p++ = get();} while (p < (uint8_t *) ((&x)+1));
      return *this;
    }

    Packet &operator>>(BD_ADDR &addr) {
      read((uint8_t *) &addr, sizeof(BD_ADDR));
      return *this;
    }

    Packet &operator>>(local_name_ptr name) {
      read((uint8_t *) name, sizeof(local_name));
      return *this;
    }

    operator uint8_t *() {return storage + position;}

    void prepare_for_tx() {
      if (position != 0) flip();

      switch (storage[0]) {
      case COMMAND_PACKET :
        seek(3);
        *this << (uint8_t) (limit - 4); // command length
        break;

      case ACL_PACKET :
        seek(3);
        *this << (uint16_t) (limit - 5); // acl length
        *this << (uint16_t) (limit - 9); // l2cap length
        break;
      }

      seek(0);
    }

    enum {
      HCI_HEADER_SIZE = 1,
      ACL_HEADER_SIZE = HCI_HEADER_SIZE + 4,
      L2CAP_HEADER_SIZE = ACL_HEADER_SIZE + 4
    };

    void begin_hci(enum packet_indicator ind) {
      reset();
      *this << (uint8_t) ind;
    }

    void begin_acl(uint16_t handle, uint8_t pb, uint8_t bc) {
      begin_hci(ACL_PACKET);
      uint16_t dummy_length = 0;
      *this << (uint16_t) ((bc << 14) + (pb << 12) + handle) << dummy_length;
    }

    void begin_l2cap(uint16_t handle, uint16_t cid) {
      begin_acl(handle, 0x02, 0x00);
      uint16_t dummy_length = 0;
      *this << dummy_length << cid;
    }

    /*
     * This function assumes the packet already contains a fully formatted
     * L2CAP packet. The packet is reset, but the L2CAP framing is preserved
     * in place, including the source/destination handle and the channel ID.
     * The position is left at the first L2CAP payload byte. Length fields are
     * not modified here and must be set (via prepare_for_tx) before the packet
     * can be sent.
     */
    void reset_l2cap() {
      reset();
      seek(L2CAP_HEADER_SIZE);
    }
    void dump();
  };

  class CommandPacket : public Packet {
    uint8_t storage[259];

  public:
    CommandPacket() : Packet(storage, sizeof(storage)) {}
  };

  class ACLPacket : public Packet {
    uint8_t storage[1000];
  public:
    ACLPacket() : Packet(storage, sizeof(storage)) {}
  };

  template<class T, unsigned int size>
  class Pool {
    T pool[size];
    Ring<T> available;

  public:
    Pool() {
      for (unsigned int i=0; i < size; ++i) {
        Ring<T> *p = (Ring<T> *) (pool + i);
        p->join(&available);
      }
    }

    T *allocate() {
      T *p = available.begin();
      if (p == available.end()) return 0;

      __asm("cpsid i");
      p->join(p);
      p->reset();
      __asm("cpsie i");

      return p;
    }

    void deallocate(T *p) {
      assert(p != 0);

      __asm("cpsid i");
      ((Ring<T> *) p)->join(&available);
      __asm("cpsie i");
    }
  };

  class Connection : public Ring<Connection> {
    uint16_t handle;
  };
};

class Channel : public Ring<Channel> {
  static Ring<Channel> channels;

 public:
  const uint16_t channel_id;
  uint16_t mtu;

  Channel(uint16_t c);
  ~Channel();

  virtual void receive(HCI::Packet *p);
  virtual void send(HCI::Packet *p);
  static Channel *find(uint16_t id);
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
    for(uint16_t i=start; i < next_handle; ++i) {
      AttributeBase *attr = all_handles[i];

      if (attr->type != type) continue;
      if (length != attr->length) continue; // e.g., UUIDs could be either 2 or 16 bytes
      if (memcmp(attr->_data, value, length)) continue;

      return i;
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
  const char *data;

 public:
  Attribute(const UUID &u) : AttributeBase(u, &data, sizeof(data)) {}
  Attribute(uint16_t u) : AttributeBase(u, &data, sizeof(data)) {}

  Attribute &operator=(const char *rhs) {data = rhs; length = strlen(rhs); return *this;};
};

class ATT_Channel : public Channel {
  void find_by_type_value(Packet *p);
  Ring<AttributeBase> attributes;

 public:
  ATT_Channel() : Channel(L2CAP::ATTRIBUTE_CID) {}
  void receive(Packet *p);
  void add(AttributeBase &attr) { attr.join(&attributes); }
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

/*
struct Server : public Channel {
  uint16_t next_handle;
  Ring<Service> services;

  void add(Service &s);

  Server() : Channel(L2CAP::ATTRIBUTE_CID) {}

  HCI::Packet *get_packet();
  virtual void receive(HCI::Packet *p);
};
*/

class BBand {
  enum {PACKET_POOL_SIZE = 4};

  UART &uart;
  IOPin &shutdown;
  Packet *rx;
  void (*rx_state)(BBand *);
  Packet *tx;
  uint8_t indicator_packet_storage[1];
  Packet indicator_packet;
  Pool<CommandPacket, 4> command_packet_pool;
  Pool<ACLPacket, 4> acl_packet_pool;
  Pool<HCI::Connection, 3> hci_connection_pool;
  Ring<Packet> incoming_packets;
  Ring<HCI::Connection> remotes;
  ATT_Channel att_channel;

  void (*event_handler)(BBand *, uint8_t event, Packet *);
  void (*command_complete_handler)(BBand *, uint16_t opcode, Packet *);

  uint8_t command_packet_budget;
  BD_ADDR bd_addr;

  GAP_Service gap;
  GATT_Service gatt;
  MyService my;

  void fill_uart();
  void drain_uart();

  // uart states
  void rx_new_packet();
  void rx_packet_indicator();
  void rx_event_header();
  void rx_acl_header();
  void rx_queue_received_packet();

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
  void uart_interrupt_handler();
  void process_incoming_packets();

  void deallocate_packet(Packet *packet);
  void send(Packet *p);
};
