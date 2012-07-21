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
  rx_new_packet();
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
  assert(controller != 0);
  Ring<Packet> &sent = controller->sent_packets();

  __asm("cpsid i");

  while (!sent.empty() && uart->can_write()) {
    Packet *tx = sent.rbegin(); // first in, first out

    while ((tx->get_remaining() > 0) && uart->can_write()) {
      uint8_t byte = tx->get();
      uart->write(&byte, 1);
    }

    if (tx->get_remaining() == 0) tx->deallocate();
  }

  if (!sent.empty()) {
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
  if (!controller->sent_packets().empty()) cause |= UART::TX;
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
    rx = controller->allocate_command_packet();
    assert(rx != 0);

    rx->set_limit(1+1+1); // indicator, event code, param length
    rx->put(ind);
    rx_state = &rx_event_header;
    break;

  case HCI::ACL_PACKET :
    rx = controller->allocate_acl_packet();
    assert(rx != 0);

    rx->set_limit(1+4);
    rx->put(ind);
    rx_state = &rx_acl_header;
    break;

  case HCI::GO_TO_SLEEP_IND :
    //printf("baseband wants to sleep!\n");
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
    printf("warning: UART fifo not empty after received packet\n");
  }

  rx->flip();
  rx->join(&controller->received_packets());
  rx_new_packet();
}


