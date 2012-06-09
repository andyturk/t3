#include <stdint.h>
#include "hal.h"
#include "bluetooth.h"

#include "utils/uartstdio.h"

IOPin led1('F', 3, IOPin::LED);
IOPin pc4('C', 4, IOPin::OUTPUT);
UART0 uart0;
UART1 uart1;
Baseband pan1323(uart1, pc4);

extern "C" int main() {
  CPU::set_clock_rate_50MHz();
  CPU::set_master_interrupt_enable(false);

  led1.configure();
  pc4.configure();
  uart0.configure();
  uart1.configure();

  UARTStdioInitExpClk(0, 115200); // UART0 is the console
  UARTprintf("console initialized\n");

  led1.initialize();
  led1.set_value(0);
  
  pan1323.initialize();

  CPU::set_master_interrupt_enable(true);

  pan1323.send(HCI::RESET);

  for(;;);
}

extern "C" void __attribute__ ((isr)) uart_1_handler() {
  uart1.interrupt_handler();
}
