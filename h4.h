#pragma once

#include "packet.h"

class UART;
class HostController;

class H4Controller {
 public:
  virtual Ring<Packet> &sent_packets() = 0;
  virtual Ring<Packet> &received_packets() = 0;
  virtual Packet *allocate_command_packet() = 0;
  virtual Packet *allocate_acl_packet() = 0;
};

class H4Tranceiver {
  UART *const uart;
  H4Controller *controller;

  Packet *rx;
  SizedPacket<1> indicator;

  void (*rx_state)(H4Tranceiver *);

  void rx_new_packet();
  void rx_packet_indicator();
  void rx_event_header();
  void rx_acl_header();
  void rx_queue_received_packet();

  void drain_uart();

 public:
  H4Tranceiver(UART *u);

  H4Controller *get_controller() const { return controller; }
  void set_controller(H4Controller *c) { controller = c; }

  void reset();
  void fill_uart();
  void uart_interrupt();
};
