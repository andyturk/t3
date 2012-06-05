#pragma once

#include <stdint.h>
#include <cstdlib>

#define assert(x) do {if(!(x)) for(;;);} while (0)

class CPU {
 public:
  static void set_clock_rate_50MHz();
  static uint32_t get_clock_rate();
  static void delay(uint32_t msec);
  static bool set_master_interrupt_enable(bool value);
};

class Peripheral {
 public:
  const void *base;
  const uint32_t id;
  const uint32_t interrupt;

  Peripheral(void *base, uint32_t id, uint32_t interrupt);
  virtual void configure();
  virtual void initialize();

  void set_interrupt_enable(bool value);
  void pend_interrupt();
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
  UART(uint32_t n);
  UART(void *base, uint32_t id);

  enum interrupt_mask {
    RX    = 0x01,
    TX    = 0x02,
    ERROR = 0x04
  };

  virtual void configure() = 0;
  virtual void initialize() {}

  void set_enable(bool value);
  void set_baud(uint32_t bps);
  void flush_rx_fifo();
  bool can_read();
  uint8_t read1();
  void write1(uint8_t c);
  bool can_write();
  size_t read(uint8_t *dst, size_t max);
  size_t write(const uint8_t *src, size_t max);
  uint32_t clear_interrupt_cause(uint32_t mask);
  void set_interrupt_sources(uint32_t mask);
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



