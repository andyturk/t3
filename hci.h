#pragma once

#include <stdint.h>
#include <stdarg.h>
#include <deque>

#include "hal.h"
#include "buffer.h"
#include "ring.h"
#include "command_defs.h"

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

    Packet &operator<<(hci_opcodes x) {
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

using namespace HCI;

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

  void (*event_handler)(BBand *, uint8_t event, Packet *);
  void (*command_complete_handler)(BBand *, uint16_t opcode, Packet *);

  uint8_t command_packet_budget;
  BD_ADDR bd_addr;

  void fill_uart();
  void drain_uart();

  void deallocate_packet(Packet *packet);
  void send(Packet *p);

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
};
