#pragma once

#include <stdint.h>

class CPU {
 public:
  static void set_clock_rate_50MHz();
  static uint32_t get_clock_rate();
  static void delay(uint32_t msec);
};

typedef uint32_t *Port;
typedef uint8_t Pin;

class UART {
 protected:
  void *uart_base;

 public:
  UART(void *base);

  virtual void configure() = 0;
  virtual void initialize() {}
          void enable(bool value);
          void set_baud(uint32_t bps);
          uint8_t get();
          void put(uint8_t c);
};

class UART0 : public UART {
 public:
  UART0();
  virtual void configure();
};

class UART1 : public UART {
 public:
  UART1();
  virtual void configure();
};



