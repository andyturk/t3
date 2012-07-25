#pragma once

#include "packet.h"
#include "pool.h"

class UART;
class HostController;

class H4Controller {
 public:
  virtual void sent(Packet *p) = 0;
  virtual void received(Packet *p) = 0;
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
  PacketPool<259, 4> command_packets;
  PacketPool<1000, 4> acl_packets;
  Ring<Packet> packets_to_send;
  Ring<Packet> packets_received;

  H4Tranceiver(UART *u);

  H4Controller *get_controller() const { return controller; }
  void set_controller(H4Controller *c) { controller = c; }

  void reset();
  void fill_uart();
  void uart_interrupt();
};
