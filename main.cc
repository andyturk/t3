#include <stdint.h>

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

extern "C" int main() {
  SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
  PinoutSet();

  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA); // UART0 will be on PA0 and PA1
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC); // UART1 Tx and RX
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF); // LED is on PF3
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ); // UART1 RTS/CTS
  SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0); // for debugging
  SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1); // clock UART1

  GPIOPinConfigure(GPIO_PA0_U0RX);
  GPIOPinConfigure(GPIO_PA1_U0TX);

  // Bluetooth uart is Tx/Rx/Rts/Cts
  GPIOPinConfigure(GPIO_PC6_U1RX);
  GPIOPinConfigure(GPIO_PC7_U1TX);
  GPIOPinConfigure(GPIO_PJ6_U1RTS);
  GPIOPinConfigure(GPIO_PJ3_U1CTS);

  GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
  GPIOPinTypeUART(GPIO_PORTC_BASE, GPIO_PIN_6 | GPIO_PIN_7);
  GPIOPinTypeUART(GPIO_PORTJ_BASE, GPIO_PIN_6 | GPIO_PIN_3);

  GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, GPIO_PIN_4);
  GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_3); // LED

  UARTFlowControlSet(UART1_BASE, UART_FLOWCONTROL_TX | UART_FLOWCONTROL_RX);

  GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_4, 0); // assert reset on the pan1323 device

  UARTStdioInitExpClk(0, 115200); // UART0 is the console
  UARTprintf("console initialized\n");

  UARTConfigSetExpClk(UART1_BASE, SysCtlClockGet(), 115200, 
                      (UART_CONFIG_WLEN_8 |
                       UART_CONFIG_STOP_ONE |
                       UART_CONFIG_PAR_NONE));
  UARTEnable(UART1_BASE);

  const uint32_t msec = 150;

  //SysCtlDelay((SysCtlClockGet()*msec)/1000);
  GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_4, GPIO_PIN_4); // de-assert reset on the pan1323 device

  GPIOPinWrite(GPIO_PORTF_BASE, 0x08, 0x08); // turn on LED

  SysCtlDelay((SysCtlClockGet()*msec)/1000);
  GPIOPinWrite(GPIO_PORTF_BASE, 0x08, 0x00); // turn off LED

  UARTCharPut(UART1_BASE, 0x01);
  UARTCharPut(UART1_BASE, 0x03);
  UARTCharPut(UART1_BASE, 0x0c);
  UARTCharPut(UART1_BASE, 0x00);


  uint8_t response[10];
  for (int i=0; i < 7; ++i) response[i] = UARTCharGet(UART1_BASE);

  GPIOPinWrite(GPIO_PORTF_BASE, 0x08, 0x08); // turn on LED
  for(;;);
}
