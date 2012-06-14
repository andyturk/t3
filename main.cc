#include <stdint.h>
#include "ringbuffer.h"
#include "hal.h"
#include "bluetooth.h"
#include "scheduler.h"

#include "utils/uartstdio.h"

IOPin led1('F', 3, IOPin::LED);
IOPin pc4('C', 4, IOPin::OUTPUT);
UART0 uart0;
UART1 uart1;
Baseband pan1323(uart1, pc4);
SysTick systick(1000);
Pan1323Bootstrap bootstrapper(pan1323);

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
  
  bootstrapper.initialize();

  Scheduler::run_forever();
}

void main_hook() {
  Baseband &bb = pan1323;
  Pan1323Bootstrap &boots = bootstrapper;
  BufferedUART &u = bb.uart;

  int foo = 1;
}

extern "C" void __attribute__ ((isr)) uart_1_handler() {
  uart1.interrupt_handler();
}

class Foo : public StateMachine, public Callable {
public:
  Foo() {
    go(this, (State) &want_foo);
  }
protected:
  virtual void want_foo() {UARTprintf("get me some foo!\n"); go(this, (State) want_bar);}
  virtual void want_bar() {UARTprintf("get me some bar!\n"); go(this, (State) want_baz);}
  virtual void want_baz() {UARTprintf("get me some baz!\n"); go(this, (State) want_foo);}

  void call() {
    StateMachine::operator()();
  }
};

class Foo2 : public Foo {
protected:
  virtual void want_bar() {
    UARTprintf("Foo2 says: get me sum bar!\n");
    go(this, (State) &want_baz);
  }
};

class Counter : public Callable {
protected:
  uint32_t count0;
  uint32_t count;
  Callable &other;

  void call() {
    UARTprintf("count = %d\n", count);
    if (count-- == 0) {
      count = count0;
      other.ready();
    } 
  }

public:
  Counter(uint32_t count, Callable &other) :
    count0(count-1),
    count(0),
    other(other)
  {
  }

};

Foo2 foo;
Counter every10(10, foo);

extern "C" void __attribute__ ((isr)) systick_handler() {
  every10.ready();
}
