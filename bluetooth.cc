#include "bluetooth.h"

HCITransport::HCITransport(UART &uart, IOPin &shutdown) :
  uart(uart), shutdown(shutdown)
{
}

void HCITransport::configure() {
  shutdown.configure();
  uart.configure();
}

void HCITransport::initialize() {
  shutdown.initialize();
  shutdown.set_value(0); // assert SHUTDOWN
  uart.initialize();
  uart.set_baud(115200);
  uart.set_enable(true);

  shutdown.set_value(1); // clear SHUTDOWN
  CPU::delay(150); // wait 150 msec
}
