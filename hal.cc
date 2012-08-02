#include "LM3S9D96.h"

#include "inc/hw_types.h"
// #include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/cpu.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/interrupt.h"
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

Peripheral::Peripheral() {
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

/*
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
*/

IOPort::IOPort(char name) {
  switch (name) {
  case 'A' :
    base = (void *) GPIOA_BASE;
    id = SYSCTL_PERIPH_GPIOA;
    interrupt = INT_GPIOA;
    break;

  case 'B' :
    base = (void *) GPIOB_BASE;
    id = SYSCTL_PERIPH_GPIOB;
    interrupt = INT_GPIOB;
    break;

  case 'C' :
    base = (void *) GPIOC_BASE;
    id = SYSCTL_PERIPH_GPIOC;
    interrupt = INT_GPIOC;
    break;

  case 'D' :
    base = (void *) GPIOD_BASE;
    id = SYSCTL_PERIPH_GPIOD;
    interrupt = INT_GPIOD;
    break;

  case 'E' :
    base = (void *) GPIOE_BASE;
    id = SYSCTL_PERIPH_GPIOE;
    interrupt = INT_GPIOE;
    break;

  case 'F' :
    base = (void *) GPIOF_BASE;
    id = SYSCTL_PERIPH_GPIOF;
    interrupt = INT_GPIOF;
    break;

  case 'G' :
    base = (void *) GPIOG_BASE;
    id = SYSCTL_PERIPH_GPIOG;
    interrupt = INT_GPIOG;
    break;

  case 'H' :
    base = (void *) GPIOH_BASE;
    id = SYSCTL_PERIPH_GPIOH;
    interrupt = INT_GPIOH;
    break;

  case 'J' :
    base = (void *) GPIOJ_BASE;
    id = SYSCTL_PERIPH_GPIOJ;
    interrupt = INT_GPIOJ;
    break;

  default  :
    base = (void *) 0xffffffff;
    id = 0;
    interrupt = 0;
    break;
  }
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
  
void UART::initialize() {
}

void UART::set_enable(bool value) {
  if (value) UARTEnable((uint32_t) base);
  else       UARTDisable((uint32_t) base);
}

void UART::set_fifo_enable(bool value) {
  if (value) UARTFIFOEnable ((uint32_t) base);
  else       UARTFIFODisable((uint32_t) base);
}

void UART::set_baud(uint32_t bps) {
  uint32_t system_clock_rate = SysCtlClockGet();

  UARTConfigSetExpClk((uint32_t) base, system_clock_rate, bps,
                      UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);
  uint32_t actual_baud, config;
  UARTConfigGetExpClk((uint32_t) base, system_clock_rate, &actual_baud, &config);
  actual_baud = actual_baud;
}

bool UART::can_read() {
  return UARTCharsAvail((uint32_t) base);
}

bool UART::can_write() {
  return UARTSpaceAvail((uint32_t) base);
}

void UART::flush_rx_fifo() {
  uint8_t dummy;
  while (can_read()) read(&dummy, 1);
}

void UART::flush_tx_buffer() {
  while (UARTBusy((uint32_t) base));
}

size_t UART::read(uint8_t *dst, size_t max) {
  size_t max0 = max;

  while (can_read() && max > 0) {
    *dst++ = UARTCharGet((uint32_t) base);
    max--;
  }

  return max0 - max;
}

size_t UART::write(const uint8_t *src, size_t max) {
  size_t max0 = max;

  while (can_write() && max > 0) {
    UARTCharPut((uint32_t) base, *src++);
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

uint32_t UART::disable_all_interrupt_sources() {
  uint32_t value = ((uart_register_map *) base)->IM;
  ((uart_register_map *) base)->IM = 0;
  return value;
}

void UART::reenable_interrupt_sources(uint32_t mask) {
  ((uart_register_map *) base)->IM = mask;
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

UART_0::UART_0() : UART(0)
{
}

void UART_0::configure() {
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA); // UART0 will be on PA0 and PA1
  SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0); // for debugging

  GPIOPinConfigure(GPIO_PA0_U0RX);
  GPIOPinConfigure(GPIO_PA1_U0TX);

  GPIOPinTypeUART(GPIOA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
}

UART_1::UART_1() : UART(1)
{
}

void UART_1::configure() {
  SysCtlPeripheralReset(SYSCTL_PERIPH_UART1);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC); // UART1 Tx and RX
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ); // UART1 RTS/CTS
  SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1); // clock UART1

  GPIOPinConfigure(GPIO_PC6_U1RX);
  GPIOPinConfigure(GPIO_PC7_U1TX);
  GPIOPinConfigure(GPIO_PJ6_U1RTS);
  GPIOPinConfigure(GPIO_PJ3_U1CTS);

  GPIOPinTypeUART(GPIOC_BASE, GPIO_PIN_6 | GPIO_PIN_7);
  GPIOPinTypeUART(GPIOJ_BASE, GPIO_PIN_6 | GPIO_PIN_3);
  UARTFlowControlSet((uint32_t) base, UART_FLOWCONTROL_TX | UART_FLOWCONTROL_RX);
}

Systick::Systick(uint32_t msec) :
  msec(msec)
{
}

void Systick::configure() {
  uint32_t clock = CPU::get_clock_rate();

  SysTickEnable();
  SysTickPeriodSet((clock/1000)*msec);
}

void Systick::initialize() {
  SysTickIntEnable();
}

ADC::ADC(uint32_t n) {
  switch (n) {
  case 0 :
    base = (void *) ADC0_BASE;
    id = SYSCTL_PERIPH_ADC0;
    interrupt = INT_ADC0;
    break;

  case 1 :
    base = (void *) ADC1_BASE;
    id = SYSCTL_PERIPH_ADC1;
    interrupt = INT_ADC1;
    break;
  }
}

void ADC::configure_sequence(sequence seq, sequence_trigger trig, sequence priority) {
  adc_register_map *reg = (adc_register_map *) base;
  uint32_t shift = seq*4; // shift for the bits that control this sample sequence
  
  // trigger event for this sample sequence.
  reg->EMUX = (reg->EMUX & ~(0xf << shift)) | (trig << shift);

  // priority for this sample sequence
  reg->SSPRI = (reg->SSPRI & ~(0xf << shift)) | (priority << shift);
}


void ADC::configure_step(sequence seq, unsigned int step,
                         control sc, input is, comparator cs)
{
  adc_register_map *reg = (adc_register_map *) base;
  uint32_t shift = step*4; // shift for the bits that control this sample sequence

  // Set the analog mux value for this step.
  reg->SS[seq].MUX = (reg->SS[seq].MUX & ~(0xf << shift)) | ((is & 0xf) << shift);

  // Set the upper bits of the analog mux value for this step.
  reg->SS[seq].EMUX = (reg->SS[seq].EMUX & ~(0xf << shift)) | ((is & 0xf00) << shift);

  // Set the control value for this step.
  reg->SS[seq].CTL = (reg->SS[seq].CTL & ~(0xf << shift)) | ((sc >> 4) << shift);

  if (cs) {
    // Program the comparator for the specified step.
    reg->SS[seq].DC = (reg->SS[seq].DC & ~(0xf << shift)) | (((0x00070000 & cs) >> 16) << shift);
    // Enable the comparator
    reg->SS[seq].OP = reg->SS[seq].OP | (1 << shift);
  } else {
    // Disable the comparator
    reg->SS[seq].OP = reg->SS[seq].OP & ~(1 << shift);
  }
}

void ADC::set_sequence_enable(sequence seq, bool value) {
  adc_register_map *reg = (adc_register_map *) base;
  if (value) {
    reg->ACTSS |= 1 << seq;
  } else {
    reg->ACTSS &= ~(1 << seq);
  }
}

void ADC::clear_interrupt(sequence seq) {
  adc_register_map *reg = (adc_register_map *) base;
  reg->ISC = 1 << seq;
}

void ADC::processor_trigger(sequence seq, bool wait, bool signal) {
  adc_register_map *reg = (adc_register_map *) base;
  uint32_t value = 0;

  if (wait) value |= WAIT;
  if (signal) value |= SIGNAL;
  value |= 1 << seq;

  reg->PSSI |= value;
}

uint32_t ADC::get_interrupt_status(sequence seq, bool masked) {
  adc_register_map *reg = (adc_register_map *) base;
  uint32_t status;

  if (masked) {
    status = reg->ISC & (0x10001 << seq);
  } else {
    status = reg->RIS & (0x10000 | (1 << seq));
    // if the comparator status bit is set, use the appropriate sequence bit
    if (status & 0x10000) {
      status |= 0xf0000;
      status &= ~(0x10000 << seq);
    }
  }

  return status;
}

uint32_t ADC::get_samples(sequence seq, uint32_t *buffer, uint32_t size) {
  adc_register_map *reg = (adc_register_map *) base;
  uint32_t count = 0;
  while (count < size && !(reg->SS[seq].FSTAT & FSTAT_EMPTY)) {
    *buffer++ = reg->SS[seq].FIFO;
    count += 1;
  }

  return count;
}



