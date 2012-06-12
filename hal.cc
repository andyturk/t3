#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/cpu.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/interrupt.h"
#include "utils/uartstdio.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"

#include "hal.h"
#include "register_defs.h"

void CPU::set_clock_rate_50MHz() {
  SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
}

uint32_t CPU::get_clock_rate() {
  return SysCtlClockGet();
}

void CPU::delay(uint32_t msec) {
  SysCtlDelay((get_clock_rate()*msec)/(3*1000));
}

bool CPU::set_master_interrupt_enable(bool value) {
  return (value ? CPUcpsie() : CPUcpsid()) != 0;
}

Peripheral::Peripheral(void *base, uint32_t id, uint32_t interrupt) :
  base(base), id(id), interrupt(interrupt)
{
}

void Peripheral::configure() {
  SysCtlPeripheralEnable(id);
}

void Peripheral::initialize() {
}

void Peripheral::set_interrupt_enable(bool value) {
  if (value) IntEnable(interrupt);
  else       IntDisable(interrupt);
}

void Peripheral::pend_interrupt() {
  IntPendSet(interrupt);
}

static uint32_t gpio_id_from_name(char name) {
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

static void *gpio_base_from_name(char name) {
  switch (name) {
  case 'A' : return (void *) GPIO_PORTA_BASE;
  case 'B' : return (void *) GPIO_PORTB_BASE;
  case 'C' : return (void *) GPIO_PORTC_BASE;
  case 'D' : return (void *) GPIO_PORTD_BASE;
  case 'E' : return (void *) GPIO_PORTE_BASE;
  case 'F' : return (void *) GPIO_PORTF_BASE;
  case 'G' : return (void *) GPIO_PORTG_BASE;
  case 'H' : return (void *) GPIO_PORTH_BASE;
  case 'J' : return (void *) GPIO_PORTJ_BASE;
  default  : return (void *) 0xffffffff;
  }
}

static uint32_t gpio_interrupt_from_name(char name) {
  switch (name) {
  case 'A' : return INT_GPIOA;
  case 'B' : return INT_GPIOB;
  case 'C' : return INT_GPIOC;
  case 'D' : return INT_GPIOD;
  case 'E' : return INT_GPIOE;
  case 'F' : return INT_GPIOF;
  case 'G' : return INT_GPIOG;
  case 'H' : return INT_GPIOH;
  case 'J' : return INT_GPIOJ;
  default  : return 0xffffffff;
  }
}

IOPort::IOPort(char name) :
  Peripheral(
    gpio_base_from_name(name),
    gpio_id_from_name(name),
    gpio_interrupt_from_name(name)
  )
{
}

IOPin::IOPin(char name, uint8_t pin, pin_type type) :
  IOPort(name), mask(0x01 << pin), type(type)
{
}

void IOPin::configure() {
  IOPort::configure();
  switch (type) {
  case OUTPUT :
  case LED :
    GPIOPinTypeGPIOOutput((uint32_t) base, mask);
    break;

  case UART :
    GPIOPinTypeUART((uint32_t) base, mask);
    break;

  default :
    break;
  }
}

void IOPin::set_value(bool value) {
  GPIOPinWrite((uint32_t) base, mask, value ? mask : 0);
}

static uint32_t uart_base(uint32_t n) {
  switch (n) {
  case 0  : return UART0_BASE;
  case 1  : return UART1_BASE;
  default : return 0;
  }
}

static uint32_t uart_id(uint32_t n) {
  switch (n) {
  case 0  : return SYSCTL_PERIPH_UART0;
  case 1  : return SYSCTL_PERIPH_UART1;
  default : return 0;
  }
}

static uint32_t uart_interrupt(uint32_t n) {
  switch (n) {
  case 0  : return INT_UART0;
  case 1  : return INT_UART1;
  default : return 0;
  }
}

UART::UART(uint32_t n) :
  Peripheral((void *) uart_base(n), uart_id(n), uart_interrupt(n))
{
}
  
void UART::set_enable(bool value) {
  UARTEnable((uint32_t) base);
}

void UART::set_baud(uint32_t bps) {
  UARTConfigSetExpClk((uint32_t) base, SysCtlClockGet(), bps,
                      UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);
}

uint8_t UART::read1() {
  return (uint8_t) UARTCharGet((uint32_t) base);
}

bool UART::can_read() {
  return UARTCharsAvail((uint32_t) base);
}

void UART::write1(uint8_t c) {
  UARTCharPut((uint32_t) base, c);
}


bool UART::can_write() {
  return UARTSpaceAvail((uint32_t) base);
}

void UART::flush_rx_fifo() {
  while (can_read()) read1();
}

void UART::flush_tx_buffer() {
  while (UARTBusy((uint32_t) base));
}

size_t UART::read(uint8_t *dst, size_t max) {
  size_t max0 = max;

  while (can_read() && max > 0) {
    *dst++ = read1();
    max--;
  }

  return max0 - max;
}

size_t UART::write(const uint8_t *src, size_t max) {
  size_t max0 = max;

  while (can_write() && max > 0) {
    write1(*src++);
    max--;
  }

  return max0 - max;
}

enum stellaris_uart_mask {
  error_mask   = 0x00000780,
  rx_mask      = 0x00000010,
  tx_mask      = 0x00000020,
  timeout_mask = 0x00000040
};

void UART::set_interrupt_sources(uint32_t hal_mask) {
  uint32_t stellaris_mask = 0;

  if (hal_mask & UART::RX)    stellaris_mask |= rx_mask | timeout_mask;
  if (hal_mask & UART::TX)    stellaris_mask |= tx_mask;
  if (hal_mask & UART::ERROR) stellaris_mask |= error_mask;

  ((uart_register_map *) base)->IM = stellaris_mask;
}

uint32_t UART::clear_interrupt_cause(uint32_t mask) {
  uart_interrupt_mask cause;
  uint32_t value = 0;

  cause.w = ((uart_register_map *) base)->MIS;

  if (cause.RXIM) value |= RX;
  if (cause.RTIM) value |= RX;
  if (cause.TXIM) value |= TX;
  if (cause.w & error_mask) value |= ERROR;

  if (mask & RX) cause.RXIM = cause.RTIM = 1;
  if (mask & TX) cause.TXIM = 1;
  if (mask & ERROR) cause.w |= error_mask;

  ((uart_register_map *) base)->MIS = cause.w;

  return value;
}

BufferedUART::BufferedUART(uint32_t n, uint8_t *rx, size_t rx_len, uint8_t *tx, size_t tx_len) :
  UART(n), rx(rx, rx_len), tx(tx, tx_len), delegate(0)
{
}

void BufferedUART::interrupt_handler() {
  uint32_t cause = clear_interrupt_cause(UART::RX | UART::TX | UART::ERROR);

  if (cause & UART::ERROR) {
    if (delegate) delegate->error_occurred(this);
  }

  if (cause & UART::TX) {
    fill_tx_fifo();
  }

  if (cause & UART::RX) {
    drain_rx_fifo();

    if (delegate) delegate->data_received(this);
  }
}

size_t BufferedUART::write(const uint8_t *src, size_t len) {
  set_interrupt_enable(false);
  len = tx.write(src, len);
  fill_tx_fifo();
  set_interrupt_enable(true);
  return len;
}

void BufferedUART::write1(uint8_t c) {
  if (!tx.write1(c)) for (;;);
  fill_tx_fifo();
}

void BufferedUART::drain_rx_fifo() {
  while (UART::can_read() && rx.write_capacity() > 0) {
    rx.write1(read1());
  }
}

void BufferedUART::fill_tx_fifo() {
  while (UART::can_write() && tx.read_capacity() > 0) {
    uint8_t byte;
    tx.read1(byte);
    UART::write1(byte);
  }

  // enable the tx interrupt if there's more data in the ringbuffer
  set_interrupt_sources(UART::RX | UART::ERROR | (tx.read_capacity() > 0 ? UART::TX : 0));
}

void BufferedUART::flush_tx_buffer() {
  while (tx.read_capacity() > 0);
  UART::flush_tx_buffer();
}

UART0::UART0() : UART(0)
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
  BufferedUART(1, rx_buffer, sizeof(rx_buffer), tx_buffer, sizeof(tx_buffer))
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

SysTick::SysTick(uint32_t msec) :
  msec(msec)
{
}

void SysTick::configure() {
  SysTickEnable();
  SysTickPeriodSet((CPU::get_clock_rate()/1000)*msec);
}

void SysTick::initialize() {
  SysTickIntEnable();
}
