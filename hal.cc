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

Peripheral::Peripheral(void *base) :
  base(base)
{
}

void Peripheral::configure() {
}

void Peripheral::initialize() {
}

uint32_t IOPort::id_from_name(char name) {
  switch (name) {
  case 'A' : return SYSCTL_PERIPH_GPIOA;
  case 'B' : return SYSCTL_PERIPH_GPIOB;
  case 'C' : return SYSCTL_PERIPH_GPIOC;
  case 'D' : return SYSCTL_PERIPH_GPIOD;
  case 'E' : return SYSCTL_PERIPH_GPIOE;
  case 'F' : return SYSCTL_PERIPH_GPIOF;
  case 'G' : return SYSCTL_PERIPH_GPIOG;
  case 'H' : return SYSCTL_PERIPH_GPIOH;
  case 'J' : return SYSCTL_PERIPH_GPIOJ;
  default  : return 0;
  }
}

IOPort::IOPort(char name) :
  Peripheral(0),
  id(id_from_name(name))
{
}

void IOPort::configure() {
  set_enable(true);
}

void IOPort::set_enable(bool value) {
  if(value) SysCtlPeripheralEnable(id);
  else      SysCtlPeripheralDisable(id);
}

IOPin::IOPin(char name, uint8_t mask) :
  IOPort(name), mask(mask)
{
}

UART::UART(void *base) :
  Peripheral(base)
{
}

void UART::enable(bool value) {
  UARTEnable((uint32_t) base);
}

void UART::set_baud(uint32_t bps) {
  UARTConfigSetExpClk((uint32_t) base, SysCtlClockGet(), bps,
                      UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);
}

uint8_t UART::get() {
  UARTCharGet((uint32_t) base);
}

void UART::put(uint8_t c) {
  UARTCharPut((uint32_t) base, c);
}

UART0::UART0() :
  UART((void *) UART0_BASE)
{
}

void UART0::configure() {
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA); // UART0 will be on PA0 and PA1
  SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0); // for debugging

  GPIOPinConfigure(GPIO_PA0_U0RX);
  GPIOPinConfigure(GPIO_PA1_U0TX);

  GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
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
  UARTFlowControlSet((uint32_t) base, UART_FLOWCONTROL_TX | UART_FLOWCONTROL_RX);
}

