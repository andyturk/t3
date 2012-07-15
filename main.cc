#include <stdint.h>
#include "assert.h"
#include "buffer.h"
#include "hal.h"
#include "hci.h"
#include "att.h"
#include "h4.h"
#include "att.h"
#include "gatt.h"
#include "screen.h"

IOPin led1('F', 3, IOPin::LED);
IOPin pc4('C', 4, IOPin::OUTPUT);
//UART0 uart0;
UART1 uart1;
BBand pan1323(uart1, pc4);
SysTick systick(1000);
H4Tranceiver h4(&uart1, &pan1323);
ATT_Channel att_channel(pan1323);
GAP_Service gap("Test Dev 1");
GATT_Service gatt;

struct MyService : public Attribute<uint16_t> {
  Characteristic<char> char_1;
  Characteristic<char> char_2;
  Characteristic<char> char_3;

  MyService() :
    Attribute<uint16_t>(GATT::PRIMARY_SERVICE, 0xfff0),
      char_1((uint16_t) 0xfff1),
      char_2((uint16_t) 0xfff2),
      char_3("00001234-0000-1000-8000-00805F9B34FB")
  {
  }

  virtual uint16_t group_end() {return char_3.handle;}
};

MyService my;

extern "C" int main() {
  CPU::set_clock_rate_50MHz();
  CPU::set_master_interrupt_enable(false);
  Screen::the_screen.initialize();

  led1.configure();
  pc4.configure();
  //uart0.configure();
  uart1.configure();
  systick.configure();

  //  UARTStdioInitExpClk(0, 115200); // UART0 is the console
  // printf("console initialized\n");

  //systick.initialize();

  led1.initialize();
  led1.set_value(0);
  CPU::set_master_interrupt_enable(true);
  
  AttributeBase::dump_attributes();
  pan1323.initialize();

  do {
    pan1323.process_incoming_packets();
    led1.set_value(1);
    asm volatile ("wfi");
  } while (true);
}

extern "C" void __attribute__ ((isr)) uart_1_handler() {
  h4.uart_interrupt();
}

