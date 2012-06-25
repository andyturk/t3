#include <stdint.h>
#include "buffer.h"
#include "hal.h"
#include "bluetooth.h"
#include "scheduler.h"
#include "packets.h"

#include "utils/uartstdio.h"

IOPin led1('F', 3, IOPin::LED);
IOPin pc4('C', 4, IOPin::OUTPUT);
UART0 uart0;
UART1 uart1;
// Baseband pan1323(uart1, pc4);
BBand pan1323(uart1, pc4);
SysTick systick(1000);
//Pan1323Bootstrap bootstrapper(pan1323);

extern "C" int main() {
  CPU::set_clock_rate_50MHz();
  CPU::set_master_interrupt_enable(false);

  led1.configure();
  pc4.configure();
  uart0.configure();
  uart1.configure();
  systick.configure();

  UARTStdioInitExpClk(0, 115200); // UART0 is the console
  UARTprintf("console initialized\n");

  //systick.initialize();

  led1.initialize();
  led1.set_value(0);
  CPU::set_master_interrupt_enable(true);
  
  pan1323.initialize();

  do {
    pan1323.process_incoming_packets();
    led1.set_value(1);
    asm volatile ("wfi");
  } while (true);
}

extern "C" void __attribute__ ((isr)) uart_1_handler() {
  pan1323.uart_interrupt_handler();
}

