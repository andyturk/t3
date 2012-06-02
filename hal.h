#pragma once

#include <stdint.h>

class CPU {
 public:
  static void set_clock_rate_50MHz();
  static uint32_t get_clock_rate();
  static void delay(uint32_t msec);
};

class Peripheral {
 public:
  const void *base;
  const uint32_t id;

  Peripheral(void *base, uint32_t id);
  virtual void configure();
  virtual void initialize();
};

class IOPort : public Peripheral {
 public:
  IOPort(char name);
};

class IOPin : public IOPort {
  const uint8_t mask;

 public:
  const enum pin_type {
    INPUT,
    OUTPUT,
    LED,
    UART
  } type;

  IOPin(char name, uint8_t pin, pin_type type);
  virtual void configure();
  void set_value(bool value);
  bool get_value();
};

class UART : public Peripheral {
 public:
  UART(void *base, uint32_t id);

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



