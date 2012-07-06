#pragma once

#include <stdint.h>
#include <stdarg.h>
#include <deque>

#include "hal.h"
#include "buffer.h"

namespace HCI {
  struct BD_ADDR {
    uint8_t data[6];
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
    Ring<Packet> available;

  public:
    Pool() {
      for (unsigned int i=0; i < size; ++i) pool[i].join(&available);
    }

    void allocate(HCI::Packet *&p) {
      assert(!available.empty());

      __asm("cpsid i");
      p = (Packet *) available.right;
      p->join(p);
      p->reset();
      __asm("cpsie i");
    }

    void deallocate(HCI::Packet *p) {
      assert(p != 0);

      __asm("cpsid i");
      p->join(&available);
      __asm("cpsie i");
    }
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
  Ring<Packet> incoming_packets;

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


