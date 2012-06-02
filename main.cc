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

  GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, GPIO_PIN_4);

  GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_4, 0); // assert reset on the pan1323 device

  UARTStdioInitExpClk(0, 115200); // UART0 is the console
  UARTprintf("console initialized\n");

  uart1.initialize();
  uart1.set_baud(115200);
  uart1.enable(true);

  const uint32_t msec = 150;

  GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_4, GPIO_PIN_4); // de-assert reset on the pan1323 device

  GPIOPinWrite(GPIO_PORTF_BASE, 0x08, 0x08); // turn on LED
  CPU::delay(150);
  GPIOPinWrite(GPIO_PORTF_BASE, 0x08, 0x00); // turn off LED

  uart1.put(0x01);
  uart1.put(0x03);
  uart1.put(0x0c);
  uart1.put(0x00);

  uint8_t response[10];
  for (int i=0; i < 7; ++i) response[i] = uart1.get();

  GPIOPinWrite(GPIO_PORTF_BASE, 0x08, 0x08); // turn on LED
  for(;;);
}
