#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/interrupt.h"
#include "utils/uartstdio.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"

#include "hal.h"

void CPU::set_clock_rate_50MHz() {
  SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
}

uint32_t CPU::get_clock_rate() {
  return SysCtlClockGet();
}

void CPU::delay(uint32_t msec) {
  SysCtlDelay((get_clock_rate()*msec)/(3*1000));
}

UART::UART(void *base) :
  uart_base(base)
{
}

void UART::enable(bool value) {
  UARTEnable((uint32_t) uart_base);
}

void UART::set_baud(uint32_t bps) {
  UARTConfigSetExpClk((uint32_t) uart_base, SysCtlClockGet(), bps,
                      UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);
}

uint8_t UART::get() {
  UARTCharGet((uint32_t) uart_base);
}

void UART::put(uint8_t c) {
  UARTCharPut((uint32_t) uart_base, c);
}

UART0::UART0() :
  UART((void *) UART0_BASE)
{
}

UART1::UART1() :
  UART((void *) UART1_BASE)
{
}

void UART1::configure() {
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC); // UART1 Tx and RX
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ); // UART1 RTS/CTS
  SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1); // clock UART1

  GPIOPinConfigure(GPIO_PC6_U1RX);
  GPIOPinConfigure(GPIO_PC7_U1TX);
  GPIOPinConfigure(GPIO_PJ6_U1RTS);
  GPIOPinConfigure(GPIO_PJ3_U1CTS);

  GPIOPinTypeUART(GPIO_PORTC_BASE, GPIO_PIN_6 | GPIO_PIN_7);
  GPIOPinTypeUART(GPIO_PORTJ_BASE, GPIO_PIN_6 | GPIO_PIN_3);
  UARTFlowControlSet((uint32_t) uart_base, UART_FLOWCONTROL_TX | UART_FLOWCONTROL_RX);
}

