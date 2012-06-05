#include "bluetooth.h"
#include "register_defs.h"

UARTBuffer::UARTBuffer(UART &uart) :
  uart(uart),
  rx(rx_storage, sizeof(rx_storage)),
  tx(tx_storage, sizeof(tx_storage))
{
}

void UARTBuffer::drain_rx_fifo() {
  while (uart.can_read() && rx.write_capacity() > 0) {
    rx.write1(uart.read1());
  }
}

void UARTBuffer::fill_tx_fifo() {
  while (uart.can_write() && tx.read_capacity() > 0) {
    uint8_t byte;
    tx.read1(byte);
    uart.write1(byte);
  }

  // enable the tx interrupt if there's more data in the ringbuffer
  uart.set_interrupt_sources(UART::RX | UART::ERROR | (tx.read_capacity() > 0 ? UART::TX : 0));
}

void UARTBuffer::interrupt_handler() {
  uint32_t cause = uart.clear_interrupt_cause(UART::RX | UART::TX | UART::ERROR);

  if (cause & UART::ERROR) {
    error();
  }

  if (cause & UART::TX) {
    fill_tx_fifo();
  }

  if (cause & UART::RX) {
    drain_rx_fifo();
    data_received();
  }
}

size_t UARTBuffer::write(const uint8_t *buffer, size_t length) {
  uart.set_interrupt_enable(false);
  size_t count = tx.write(buffer, length);
  fill_tx_fifo();
  uart.set_interrupt_enable(true);
  return count;
}

HCI::HCI(UART &uart, IOPin &shutdown) :
  UARTBuffer(uart), shutdown(shutdown)
{
}

void HCI::configure() {
  shutdown.configure();
  uart.configure();
}

void HCI::initialize() {
  shutdown.initialize();
  shutdown.set_value(0); // assert SHUTDOWN
  uart.initialize();
  uart.set_baud(115200);
  uart.set_enable(true);
  uart.set_interrupt_enable(true);

  shutdown.set_value(1); // clear SHUTDOWN
  CPU::delay(150); // wait 150 msec
}



