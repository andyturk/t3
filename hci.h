#pragma once

#include <stdint.h>
#include <stdarg.h>
#include <deque>

#include "hal.h"
#include "buffer.h"

extern const char hex_digits[16];

namespace L2CAP {
  enum fixed_channel_ids {
    SIGNALLING_CID       = 0x0001,
    CONNECTIONLESS_CID   = 0x0002,
    AMP_MANAGER_CID      = 0x0003,
    ATTRIBUTE_CID        = 0x0004,
    LE_SIGNALLING_CID    = 0x0005,
    SECURITY_MANAGER_CID = 0x0006,
    AMP_TEST_MANAGER_CID = 0x003f
  };
};

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

  template<typename T>
  struct Ring {
    Ring *left;
    Ring *right;

    Ring() {left = right = this;}
    bool empty() const {return left == this && right == this;}

    void join(Ring *other) { // join right
      // remove this from its current ring
      left->right = this->right;
      right->left = this->left;

      // form a singleton ring so that x->join(x) works
      left = right = this;

      // set up local pointers
      this->right = other->right;
      this->left = other;

      // splice this into the other ring
      other->right->left = this;
      other->right = this;
    }
  };

  class Packet : public Ring<Packet>, public FlipBuffer<uint8_t> {
    void vfput(const char *format, va_list args);

  public:
    Packet(uint8_t *buf, size_t len) : FlipBuffer<uint8_t>(buf, len) {}
    void command(uint16_t opcode, const char *format = 0, ...);
    void fget(const char *format, ...);
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
      if (available.empty()) return 0;

      __asm("cpsid i");
      T *p = (T *) available.right;
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
  void acl_packet_handler(uint16_t handle, uint8_t flags, Packet *p);
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

#include "command_defs.h"


