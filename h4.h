#pragma once

#include "packet.h"

class UART;
class HostController;

class H4Tranceiver {
  UART *const uart;
  HostController *const controller;

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
  H4Tranceiver(UART *u, HostController *c);

  void reset();
  void fill_uart();
  void uart_interrupt();
};
