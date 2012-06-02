#include <stdint.h>
#include "hal.h"

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/interrupt.h"
#include "utils/uartstdio.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
extern "C" {
#include "drivers/set_pinout.h"
};

IOPin pf3('F', 3, IOPin::LED);
IOPin pc4('C', 4, IOPin::OUTPUT);
UART0 uart0;
UART1 uart1;

extern "C" int main() {
  CPU::set_clock_rate_50MHz();
  uart1.configure();
  uart0.configure();
  pf3.configure();
  pc4.configure();

  pc4.set_value(0); // assert reset on pan 1323

  UARTStdioInitExpClk(0, 115200); // UART0 is the console
  UARTprintf("console initialized\n");

  uart1.initialize();
  uart1.set_baud(115200);
  uart1.enable(true);

  const uint32_t msec = 150;

  pc4.set_value(1); // de-assert reset on pan1323

  pf3.set_value(1); // turn on LED
  CPU::delay(150);
  pf3.set_value(0); // turn off LED

  uart1.put(0x01);
  uart1.put(0x03);
  uart1.put(0x0c);
  uart1.put(0x00);

  uint8_t response[10];
  for (int i=0; i < 7; ++i) response[i] = uart1.get();

  pf3.set_value(1); // turn on LED
  for(;;);
}
