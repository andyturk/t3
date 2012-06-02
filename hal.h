#pragma once

#include <stdint.h>

class CPU {
 public:
  static void set_clock_rate_50MHz();
  static uint32_t get_clock_rate();
  static void delay(uint32_t msec);
};

class Peripheral {
 protected:
  const void *base;

 public:
  Peripheral(void *base);
  virtual void configure();
  virtual void initialize();
};

class IOPort : public Peripheral {
 protected:
  const uint32_t id;
  static uint32_t id_from_name(char name);

 public:
  IOPort(char name);
  virtual void configure();
  virtual void set_enable(bool value);
};

class IOPin : public IOPort {
  const uint8_t mask;

 public:
  IOPin(char name, uint8_t mask);
};

typedef uint32_t *Port;
typedef uint8_t Pin;

class UART : public Peripheral {
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



