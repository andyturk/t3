#include "assert.h"
#include "h4.h"
#include "hal.h"
#include "hci.h"

H4Tranceiver::H4Tranceiver(UART *u) :
  uart(u),
  controller(0),
  rx(0),
  rx_state(0)
{
  reset();
}

void H4Tranceiver::reset() {
  command_packets.reset(); // all command packets are on the free list
  acl_packets.reset();     // same for acl packets
  packets_to_send.join(&packets_to_send); // clear the send queue
  packets_received.join(&packets_received); // clear the receive queue

  rx_new_packet(); // start looking for a new packet
}

void H4Tranceiver::wait_for_packets() {
  while (packets_received.empty()) asm volatile ("wfi");
}

void H4Tranceiver::drain_uart() {
  assert(rx == 0 || rx->get_remaining() > 0);

  while (rx && (rx->get_remaining() > 0) && uart->can_read()) {
    uint8_t byte;
    uart->read(&byte, 1);
    rx->put(byte);
    if (rx->get_remaining() == 0) rx_state(this);
  }
}

void H4Tranceiver::fill_uart() {
  __asm("cpsid i");

  while (!packets_to_send.empty() && uart->can_write()) {
    Packet *tx = packets_to_send.rbegin(); // first in, first out

    while ((tx->get_remaining() > 0) && uart->can_write()) {
      uint8_t byte = tx->get();
      uart->write(&byte, 1);
    }

    if (tx->get_remaining() == 0) {
      if (controller) controller->sent(tx);
      tx->deallocate();
    }
  }

  if (!packets_to_send.empty()) {
    uart->set_interrupt_sources(UART::RX | UART::TX | UART::ERROR);
  }

  __asm("cpsie i");
}

void H4Tranceiver::uart_interrupt() {
  uint32_t cause = uart->clear_interrupt_cause(UART::RX | UART::TX | UART::ERROR);

  assert(!(cause & UART::ERROR));

  if (cause & UART::TX) fill_uart();
  if (cause & UART::RX) drain_uart();

  // we always care about received data and errors
  cause = UART::RX | UART::ERROR;

  // but only enable the tx interrupt if there's data to send
  if (!packets_to_send.empty()) cause |= UART::TX;
  uart->set_interrupt_sources(cause);
}

void H4Tranceiver::rx_new_packet() {
  indicator.reset();
  indicator.set_limit(1);
  rx = &indicator;
  rx_state = &rx_packet_indicator;
}

void H4Tranceiver::rx_packet_indicator() {
  uint8_t ind = indicator.peek(-1);

  switch (ind) {
  case HCI::EVENT_PACKET :
    rx = command_packets.allocate();
    assert(rx != 0);

    rx->set_limit(1+1+1); // indicator, event code, param length
    rx->put(ind);
    rx_state = &rx_event_header;
    break;

  case HCI::ACL_PACKET :
    rx = acl_packets.allocate();
    assert(rx != 0);

    rx->set_limit(1+4);
    rx->put(ind);
    rx_state = &rx_acl_header;
    break;

  case HCI::GO_TO_SLEEP_IND :
    //debug("baseband wants to sleep!\n");
    rx->reset();
    break;

  case HCI::COMMAND_PACKET :
  case HCI::SYNCHRONOUS_DATA_PACKET :
  default :
    assert(false);
  }
}

void H4Tranceiver::rx_event_header() {
  uint8_t param_length = rx->peek(-1);

  if (param_length > 0) {
    rx->set_limit(1+1+1 + param_length);
    rx_state = &rx_queue_received_packet;
  } else {
    // there are no params, so just chain to the next state
    rx_queue_received_packet();
  }
}

void H4Tranceiver::rx_acl_header() {
  uint16_t length = (rx->peek(-1) << 8) + (rx->peek(-2));
  rx->set_limit(1+4+length);
  rx_state = &rx_queue_received_packet;
}

void H4Tranceiver::rx_queue_received_packet() {
  // assert that we're handling the uart interrupt so we won't
  // be interrupted

  if (uart->can_read()) {
    debug("warning: UART fifo not empty after received packet\n");
  }

  rx->flip();
  rx->join(&packets_received);
  if (controller) controller->received(rx);
  rx_new_packet();
}


