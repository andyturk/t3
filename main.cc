#include <stdint.h>
#include "hal.h"
#include "bluetooth.h"

#include "utils/uartstdio.h"
#include "hci.h"

IOPin led1('F', 3, IOPin::LED);
IOPin pc4('C', 4, IOPin::OUTPUT);
UART0 uart0;
UART1 uart1;
Baseband pan1323(uart1, pc4);

extern "C" int main() {
  CPU::set_clock_rate_50MHz();

  CPU::set_master_interrupt_enable(false);

  uart0.configure();
  UARTStdioInitExpClk(0, 115200); // UART0 is the console
  UARTprintf("console initialized\n");

  led1.configure();
  led1.initialize();
  led1.set_value(0);

  pan1323.configure();
  pan1323.initialize();

  CPU::set_master_interrupt_enable(true);

  uint8_t reset_cmd[] = {0x01, 0x03, 0x0c, 0x00};
  pan1323.uart.write(reset_cmd, sizeof(reset_cmd));

  /*
  uart1.write1(0x01);
  uart1.write1(0x03);
  uart1.write1(0x0c);
  uart1.write1(0x00);
  */

  bool switch0 = false;
  uint8_t response[10];
  int i;

  do {
    //size_t tx_r_cap = pan1323.uart.tx.read_capacity();
    //size_t rx_r_cap = pan1323.uart.rx.read_capacity();
    bool can_read = uart1.can_read();
    bool can_write = uart1.can_write();

    if (switch0) uart1.pend_interrupt();
      //for (i=0; i < 7; ++i) response[i] = uart1.read1();
      //can_read = uart1.can_read();

    led1.set_value(1);
    led1.set_value(0);
  } while (uart1.can_read());

  led1.set_value(1); // turn on LED

  for(;;);
}

extern "C" void __attribute__ ((isr)) uart_1_handler() {
  uart1.interrupt_handler();
}
