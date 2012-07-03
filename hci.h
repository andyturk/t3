#pragma once

#include <stdint.h>
#include <stdarg.h>

#include "hal.h"
#include "buffer.h"

namespace HCI {
  struct BD_ADDR {
    uint8_t data[6];
  };

  class Packet : public FlipBuffer<uint8_t> {
    enum {MAX_PACKET = 259};
    uint8_t buf[MAX_PACKET];

    void vfput(const char *format, va_list args);

  public:
    Packet *next;

    Packet() : FlipBuffer<uint8_t>(buf, MAX_PACKET), next(0) {}
    void command(uint16_t opcode, const char *format = 0, ...);
    void fget(const char *format, ...);
  };

};

class BBand {
  enum {PACKET_POOL_SIZE = 4};

  UART &uart;
  IOPin &shutdown;
  HCI::Packet *rx;
  void (*rx_state)(BBand *);
  HCI::Packet *tx;
  HCI::Packet packet_pool[PACKET_POOL_SIZE];
  HCI::Packet *free_packets;
  HCI::Packet *incoming_packets;

  void (*event_handler)(BBand *, uint8_t event, HCI::Packet *);
  void (*command_complete_handler)(BBand *, uint16_t opcode, HCI::Packet *);

  uint8_t command_packet_budget;
  HCI::BD_ADDR bd_addr;

  void fill_uart();
  void drain_uart();

  HCI::Packet *allocate_packet();
  void deallocate_packet(HCI::Packet *packet);
  void send(HCI::Packet *p);

  // uart states
  void rx_expect_packet_indicator();
  void rx_expect_event_code_and_length();
  void rx_expect_event_parameters();

  // initialization states
  void cold_boot(uint16_t opcode, HCI::Packet *p);
  void upload_patch(uint16_t opcode, HCI::Packet *p);
  void warm_boot(uint16_t, HCI::Packet *p);

  void standard_packet_handler(HCI::Packet *p);

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


